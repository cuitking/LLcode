#ifndef SCGL_TABLE_H
#define SCGL_TABLE_H

#include <map>
#include "Variant.h"

typedef struct lua_State lua_State;

namespace scgl
{

	class CTable
	{
	public:
		typedef std::map<CVariant, CVariant> VariantMap;
	public:
		/**
		 * 构造一个table.
		 * @param tableName 此table的名字，默认为"noname"
		 */
		explicit CTable(const char* tableName = ("noname"));
		CTable(const CTable& rhs);
		~CTable();
		const CTable& operator=(const CTable& rhs);
		bool operator==(const CTable& rhs) const;
	public:
		/**
		 * 读取key对应的value，如果不存在此key，则返回CVariant::NIL.
		 * stl的map里不提供此函数，不提供的主要原因是当使用不存在的key时会自动插入一组数据，
		 * 而const对象不允许修改，因此此函数无法实现。但是在此处由于定义了CVariant::NIL
		 * ，就可以让不存在的对象返回CVariant::NIL。
		 * 这里有一个问题，const对象调用[]函数时，对象没有任何改变。而非const对象调用[]时则会
		 * 插入一组新数据，也就是同一个函数会造成不同的结果。所以为了接口的一致性，只提供读的[]函数。
		 * 写操作由Insert提供。
		 */
		template <typename K>
		const CVariant& operator[](const K& key) const;
		/**
		 * 插入一组元素.
		 * 这是对此类写的唯一办法。
		 */
		template <typename K, typename V>
		void Insert(const K& key, const V& value);
		/**
		 * 判断是否存在以key为主键的元素
		 */
		template <typename K>
		bool IsExist(const K& key) const;

	public:
		//tstring GetName() const;
		/**
		 * 设定此table的名字
		 */
		void SetName(const char*);
		/**
		 * 取得此Table中元素的数量
		 */
		size_t Size() const;
		/**
		 * 删除Table中的所有元素
		 */
		void Clear();
		/**
		 * 将此Table压入lua中
		 */
		void PushIntoLua(lua_State* L) const;
		/**
		 * 将此Table压入lua中
		 */
		const VariantMap& GetAllValue() const;

	public:

	private:
		template <typename K>
		VariantMap::const_iterator Find(const K& key) const;
		template <typename K>
		VariantMap::iterator Find(const K& key);
		void PushTable(lua_State* L, const CTable& table) const;
		void PushValue(lua_State* L, const CVariant& variant) const;

	private:
		VariantMap	m_table;
		char		m_tableName[32];
	};

	template <typename K, typename V>
	void CTable::Insert(const K& key, const V& value)
	{
		if (CVariant::NIL == value)
		{
			m_table.erase(key);
			return;
		}

		VariantMap::iterator itExist = Find(key);
		if (itExist == m_table.end())
		{
			m_table.insert(std::make_pair(CVariant(key), CVariant(value)));
		}
		else
		{
			itExist->second = value;
		}
	}

	template <typename K>
	const CVariant& CTable::operator[](const K& key) const
	{
		VariantMap::const_iterator itExist = Find(key);
		if (itExist == m_table.end())
		{
			return CVariant::NIL;
		}
		return itExist->second;
	}

	template <typename K>
	bool CTable::IsExist(const K& key) const
	{
		if (Find(key) == m_table.end())
		{
			return false;
		}
		return true;
	}

	template <typename K>
	CTable::VariantMap::const_iterator CTable::Find(const K& key) const
	{
		return m_table.find(key);
	}

	template <typename K>
	CTable::VariantMap::iterator CTable::Find(const K& key)
	{
		return m_table.find(key);
	}

}

#endif
