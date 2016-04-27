#ifndef SCGL_VARIANT_H
#define SCGL_VARIANT_H

#include <vector>
#include <boost/noncopyable.hpp>

// 为了性能，本类不允许包含非POD的数据成员

typedef struct lua_State lua_State;
namespace scgl
{

	class CTable;
	class CVariant : boost::noncopyable
	{
	public:
		enum ValueType
		{
			TYPE_NIL,
			TYPE_INTEGER,			// 所有整型都向上转为long long，只要不溢出
			TYPE_NUMBER,			// 所有浮点数向上转为double
			TYPE_BOOLEAN,
			TYPE_STRING,
			TYPE_TABLE,
			TYPE_LIGHTUSERDATA,
		};

	public:
		CVariant();
		CVariant(const CVariant& rhs);
		CVariant(bool boolean);
		CVariant(char number);
		CVariant(unsigned char number);
		CVariant(short number);
		CVariant(unsigned short number);
		CVariant(int number);
		CVariant(unsigned int number);
		CVariant(long number);
		CVariant(unsigned long number);
		CVariant(long long number);
		CVariant(float number);
		CVariant(double number);
		CVariant(const char* string);
		CVariant(const std::string& string);
		CVariant(const CTable& table);
		CVariant(const void* userdata);
		~CVariant();

	public:
		const CVariant& operator=(const CVariant& rhs);
		const CVariant& operator=(bool boolean);
		const CVariant& operator=(char number);
		const CVariant& operator=(unsigned char number);
		const CVariant& operator=(short number);
		const CVariant& operator=(unsigned short number);
		const CVariant& operator=(int number);
		const CVariant& operator=(unsigned int number);
		const CVariant& operator=(long number);
		const CVariant& operator=(unsigned long number);
		const CVariant& operator=(long long number);
		const CVariant& operator=(float number);
		const CVariant& operator=(double number);
		const CVariant& operator=(const char* string);
		const CVariant& operator=(const std::string& string);
		const CVariant& operator=(const CTable& table);
		const CVariant& operator=(const void* userdata);

	public:
		bool operator==(const CVariant& rhs) const;
		bool operator==(bool boolean) const;
		bool operator==(char number) const;
		bool operator==(unsigned char number) const;
		bool operator==(short number) const;
		bool operator==(unsigned short number) const;
		bool operator==(int number) const;
		bool operator==(unsigned int number) const;
		bool operator==(long number) const;
		bool operator==(unsigned long number) const;
		bool operator==(long long number) const;
		bool operator==(float number) const;
		bool operator==(double number) const;
		bool operator==(const char* string) const;
		bool operator==(const std::string& string) const;
		bool operator==(const CTable& table) const;
		bool operator==(const void* userdata) const;

	public:
		bool operator<(const CVariant& rhs) const;

	public:
		template <typename T>
		bool operator!=(const T& t) const;
		ValueType GetMetaType() const;
		bool IsNumber() const;

		template <typename T>
		T GetNumber() const
		{
			if (m_type == TYPE_INTEGER)
			{
				return static_cast<T>(m_integer);
			}
			else if (m_type == TYPE_NUMBER)
			{
				return static_cast<T>(m_number);
			}
			throw "bad_type in Variant::GetNumber";
		}

		bool GetBoolean() const;
		const char* GetString() const;
		const CTable& GetTable() const;
		const void* GetLightUserdata() const;
		void Clear();
		int GetLength() const;

	public:
		const static CVariant   NIL;

	private:
		friend class CTable;
		void SetValue(const CVariant& rhs);
		CVariant(lua_State* thread);
		const CVariant& operator=(lua_State* thread);
		bool operator==(lua_State* thread) const;

	private:
		ValueType   m_type;
		union
		{
			long long			m_integer;
			double				m_number;
			bool				m_boolean;
			const void*			m_lightuserdata;
			const CTable*		m_pTable;
			char				m_string[56];
		};
		int	m_length;
	};

	template <typename T>
	bool CVariant::operator!=(const T& t) const
	{
		return !(*this == t);
	}

	typedef std::vector<CVariant> VariantVector;

}

#endif
