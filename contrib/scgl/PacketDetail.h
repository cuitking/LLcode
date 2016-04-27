#ifndef SCGL_PACKET_DETAIL_H
#define SCGL_PACKET_DETAIL_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>

namespace scgl
{

struct SPacketDetail
{
	int packetID;
	int packetWay;
	std::string packetName;
	int packetFilter;
	std::string pbName;
	SPacketDetail(int _packetID, int _packetWay, const char* _packetName, int _packetFilter, const char* _pbName)
		: packetID(_packetID)
		, packetWay(_packetWay)
		, packetName(_packetName)
		, packetFilter(_packetFilter)
		, pbName(_pbName)
	{

	}
};

namespace bm = boost::multi_index;
struct SPacketID {};
struct SPacketName {};
typedef boost::multi_index_container <
SPacketDetail,
bm::indexed_by <
bm::hashed_unique <
bm::tag<SPacketID>,
bm::member<SPacketDetail, int, &SPacketDetail::packetID>
> ,
bm::hashed_unique <
bm::tag<SPacketName>,
bm::member<SPacketDetail, std::string, &SPacketDetail::packetName>
>
>
> PacketDetailMap;

}

#endif