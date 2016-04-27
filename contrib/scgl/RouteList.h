#ifndef ROUT_LIST_H
#define ROUT_LIST_H

#include "Packet.h"
#include <map>
#include <string>
#include <boost/bimap.hpp>


namespace scgl
{
	//typedef std::map<Guid, bool> GuidMap;

	class CRouteList
	{
	public:
		CRouteList(void);
		virtual ~CRouteList(void);
		void Add(unsigned long long dest, unsigned long long mid, Guid midGuid);
		void Delete(const unsigned long long connect);
	private:
		typedef std::map<unsigned long long, Guid> StringGuidMap;  //<midname, guids>
		typedef boost::bimap<unsigned long long, unsigned long long> RoutMap;  // <dest, mid>
		RoutMap			m_routList;
		StringGuidMap	m_midMap;
	};

}
#endif