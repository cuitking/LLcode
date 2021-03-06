﻿#ifndef PACKET_WAY_H
#define PACKET_WAY_H

enum EPACKET_WAY
{
	PACKET_WAY_NOT_DEFINED	= 0,
	PACKET_WAY_TO_PLAYER	= 1,
	PACKET_WAY_TO_AUTH		= 4,
	PACKET_WAY_TO_LOBBY		= 5,
	PACKET_WAY_TO_LIST		= 7,
	PACKET_WAY_TO_BATTLE	= 6,
	PACKET_WAY_TO_PLAYER_LOBBY = 8, //发给玩家所在lobby上的所有其他玩家
	PACKET_WAY_TO_PLAYER_BATTLE = 9, //发给玩家所在battle上的所有其他玩家
	PACKET_WAY_TO_PLAYER_ROOM = 10,
	PACKET_WAY_TO_GATE, //用于在gate上手动转发给player
	PACKET_WAY_TO_RELATIONSVR,

};

#endif