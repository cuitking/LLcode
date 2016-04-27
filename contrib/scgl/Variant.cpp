#include "Variant.h"
#include "Utility.h"
#include "Table.h"
#include "StringAlgorithm.h"

namespace scgl
{

	const CVariant CVariant::NIL;

	CVariant::CVariant()
		: m_type(TYPE_NIL)
	{
	}

	CVariant::CVariant(const CVariant& rhs)
		: m_type(TYPE_NIL)
		, m_pTable(NULL)
	{
		SetValue(rhs);
	}

	CVariant::CVariant(bool boolean)
		: m_type(TYPE_BOOLEAN)
		, m_boolean(boolean)
	{}

	CVariant::CVariant(char number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(unsigned char number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(short number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(unsigned short number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(int number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(unsigned int number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(long number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(unsigned long number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(long long number)
		: m_type(TYPE_INTEGER)
		, m_integer(number)
	{}

	CVariant::CVariant(float number)
		: m_type(TYPE_NUMBER)
		, m_number(number)
	{}

	CVariant::CVariant(double number)
		: m_type(TYPE_NUMBER)
		, m_number(number)
	{}

	CVariant::CVariant(const char* string)
		: m_type(TYPE_STRING)
	{
		m_length = scgl::CopyStringSafely(m_string, string);
	}

	CVariant::CVariant(const std::string& string)
		: m_type(TYPE_STRING)
	{
		m_length = scgl::CopyStringSafely(m_string, string.c_str());
	}

	CVariant::CVariant(const CTable& table)
		: m_type(TYPE_TABLE)
	{
		m_pTable = new CTable(table);
	}

	CVariant::CVariant(const void* userdata)
		: m_type(TYPE_LIGHTUSERDATA)
		, m_lightuserdata(userdata)
	{}

	CVariant::~CVariant(void)
	{
		if (m_type == TYPE_TABLE)
		{
			delete m_pTable;
		}
	}

	const CVariant& CVariant::operator=(const CVariant& rhs)
	{
		if (this == &rhs)
		{
			return *this;
		}
		SetValue(rhs);
		return *this;
	}

	const CVariant& CVariant::operator=(bool boolean)
	{
		m_type = TYPE_BOOLEAN;
		m_boolean = boolean;
		return *this;
	}

	const CVariant& CVariant::operator=(char number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(unsigned char number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(short number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(unsigned short number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(int number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(unsigned int number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(long number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(unsigned long number)
	{
		return operator=(static_cast<long long>(number));
	}

	const CVariant& CVariant::operator=(long long number)
	{
		m_type = TYPE_INTEGER;
		m_integer = number;
		return *this;
	}

	const CVariant& CVariant::operator=(float number)
	{
		return operator=(static_cast<double>(number));
	}

	const CVariant& CVariant::operator=(double number)
	{
		m_type = TYPE_NUMBER;
		m_number = number;
		return *this;
	}

	const CVariant& CVariant::operator=(const char* string)
	{
		m_type = TYPE_STRING;
		m_length = scgl::CopyStringSafely(m_string, string);
		return *this;
	}

	const CVariant& CVariant::operator=(const std::string& string)
	{
		return (operator=)(string.c_str());
	}

	const CVariant& CVariant::operator=(const CTable& table)
	{
		if (m_type == TYPE_TABLE && m_pTable != NULL)
		{
			delete m_pTable;
		}

		m_type = TYPE_TABLE;
		m_pTable = new CTable(table);
		return *this;
	}

	const CVariant& CVariant::operator=(const void* userdata)
	{
		m_type = TYPE_LIGHTUSERDATA;
		m_lightuserdata = userdata;
		return *this;
	}

	bool CVariant::operator==(const CVariant& rhs) const
	{
		if (m_type != rhs.m_type)
		{
			return false;
		}
		switch (m_type)
		{
		case TYPE_NIL:
			return m_type == rhs.m_type;
		case TYPE_INTEGER:
			return m_integer == rhs.m_integer;
		case TYPE_NUMBER:
			return m_number == rhs.m_number;
		case TYPE_BOOLEAN:
			return m_boolean == rhs.m_boolean;
		case TYPE_STRING:
			return strcmp(m_string, rhs.m_string) == 0;
		case TYPE_TABLE:
			return m_pTable == rhs.m_pTable;
		case TYPE_LIGHTUSERDATA:
			return m_lightuserdata == rhs.m_lightuserdata;
		}
		return false;
	}

	bool CVariant::operator==(bool boolean) const
	{
		return m_type == TYPE_BOOLEAN && m_boolean == boolean;
	}

	bool CVariant::operator==(char number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(unsigned char number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(short number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(unsigned short number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(int number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(unsigned int number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(long number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(unsigned long number) const
	{
		return operator==(static_cast<long long>(number));
	}

	bool CVariant::operator==(long long number) const
	{
		return m_type == TYPE_INTEGER && m_integer == number;
	}

	bool CVariant::operator==(float number) const
	{
		return operator==(static_cast<double>(number));
	}

	bool CVariant::operator==(double number) const
	{
		return m_type == TYPE_NUMBER && m_number == number;
	}

	bool CVariant::operator==(const char* string) const
	{
		return m_type == TYPE_STRING && strcmp(m_string, string) == 0;
	}

	bool CVariant::operator==(const std::string& string) const
	{
		return operator==(string.c_str());
	}

	bool CVariant::operator==(const CTable& table) const
	{
		return m_type == TYPE_TABLE && m_pTable == &table;
	}

	bool CVariant::operator==(const void* userdata) const
	{
		return m_type == TYPE_LIGHTUSERDATA && m_lightuserdata == userdata;
	}

	bool CVariant::operator<(const CVariant& rhs) const
	{
		if (m_type != rhs.m_type)
		{
			return this < &rhs;
		}

		switch (rhs.m_type)
		{
		case TYPE_INTEGER:
			return m_integer < rhs.m_integer;
		case TYPE_NUMBER:
			return m_number < rhs.m_number;
		case TYPE_BOOLEAN:
			return m_boolean < rhs.m_boolean;
		case TYPE_STRING:
			return strcmp(m_string, rhs.m_string) < 0;
		case TYPE_TABLE:
			return m_pTable < rhs.m_pTable;
		case TYPE_LIGHTUSERDATA:
			return m_lightuserdata < rhs.m_lightuserdata;
		default:
			return this < &rhs;
		}
	}

	CVariant::ValueType CVariant::GetMetaType() const
	{
		return m_type;
	}

	bool CVariant::IsNumber() const
	{
		return m_type == TYPE_INTEGER || m_type == TYPE_NUMBER;
	}

	bool CVariant::GetBoolean() const
	{
		return m_boolean;
	}

	const char* CVariant::GetString() const
	{
		return m_string;
	}

	const CTable& CVariant::GetTable() const
	{
		return *m_pTable;
	}

	const void* CVariant::GetLightUserdata() const
	{
		return m_lightuserdata;
	}

	void CVariant::Clear()
	{
		if (m_type == TYPE_TABLE && m_pTable != NULL)
		{
			delete m_pTable;
		}
		m_pTable = NULL;
		m_type = TYPE_NIL;
	}

	void CVariant::SetValue(const CVariant& rhs)
	{
		switch (rhs.m_type)
		{
		case TYPE_NIL:
			break;
		case TYPE_INTEGER:
			m_integer = rhs.m_integer;
			break;
		case TYPE_NUMBER:
			m_number = rhs.m_number;
			break;
		case TYPE_BOOLEAN:
			m_boolean = rhs.m_boolean;
			break;
		case TYPE_STRING:
			m_length = scgl::CopyStringSafely(m_string, rhs.m_string);
			break;
		case TYPE_TABLE:
		{
			if (m_type == TYPE_TABLE && m_pTable != NULL)
			{
				delete m_pTable;
			}
			m_pTable = new CTable(*rhs.m_pTable);
		}
		break;
		case TYPE_LIGHTUSERDATA:
			m_lightuserdata = rhs.m_lightuserdata;
			break;
		default:
			break;
		}
		m_type = rhs.m_type;
	}

	int CVariant::GetLength() const
	{
		if (m_type == TYPE_STRING)
		{
			return m_length;
		}
		throw std::logic_error("GetLength must be string type");
	}

}
