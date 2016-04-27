#include "Table.h"
#include "Utility.h"
#include "StringAlgorithm.h"
#include <lua/lua.hpp>

namespace scgl
{

	CTable::CTable(const char* tableName)
	{
		scgl::CopyStringSafely(m_tableName, tableName);
	}

	CTable::~CTable()
	{
	}

	CTable::CTable(const CTable& rhs)
		: m_table(rhs.m_table)
	{
		scgl::CopyStringSafely(m_tableName, rhs.m_tableName);
	}

	const CTable& CTable::operator=(const CTable& rhs)
	{
		scgl::CopyStringSafely(m_tableName, rhs.m_tableName);
		m_table = rhs.m_table;
		return *this;
	}

	bool CTable::operator==(const CTable& rhs) const
	{
		return strcmp(m_tableName, rhs.m_tableName) == 0
			   && m_table == rhs.m_table;
	}

	void CTable::SetName(const char* tableName)
	{
		scgl::CopyStringSafely(m_tableName, tableName);
	}

	size_t CTable::Size() const
	{
		return m_table.size();
	}

	void CTable::Clear()
	{
		m_table.clear();
	}

	void CTable::PushIntoLua(lua_State* L) const
	{
		PushTable(L, *this);
	}

	void CTable::PushTable(lua_State* L, const CTable& table) const
	{
		lua_newtable(L);
		for (VariantMap::const_iterator it = table.m_table.begin(); it != table.m_table.end(); ++it)
		{
			PushValue(L, it->first);
			PushValue(L, it->second);
			lua_settable(L, -3);
		}
	}

	void CTable::PushValue(lua_State* L, const CVariant& variant) const
	{
		switch (variant.GetMetaType())
		{
		case CVariant::TYPE_NIL:
			lua_pushnil(L);
			break;
		case CVariant::TYPE_INTEGER:
			//lua_pushinteger(L, variant.GetNumber<lua_Integer>());
			//break;
		case CVariant::TYPE_NUMBER:
			lua_pushnumber(L, variant.GetNumber<lua_Number>());
			break;
		case CVariant::TYPE_BOOLEAN:
			lua_pushboolean(L, variant.GetBoolean());
			break;
		case CVariant::TYPE_STRING:
			lua_pushstring(L, variant.GetString());
			break;
		case CVariant::TYPE_TABLE:
			PushTable(L, variant.GetTable());
			break;
		case CVariant::TYPE_LIGHTUSERDATA:
			lua_pushlightuserdata(L, (void*)variant.GetLightUserdata());
			break;
		default:
			lua_pushnil(L);
		}
	}

	const CTable::VariantMap& CTable::GetAllValue() const
	{
		return m_table;
	}


}
