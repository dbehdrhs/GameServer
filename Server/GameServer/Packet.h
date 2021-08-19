#pragma once

#include "PacketEnum.h"

#pragma pack(push, 1)

////////////////////////////
// Packet Header
struct PACKET_HEADER
{
	unsigned short wPacketSize;
	PACKET_ID packetID;

	PACKET_HEADER(PACKET_ID packetID = PACKET_MAX)
		: wPacketSize(0), packetID(packetID)
	{
	}
};

////////////////////////////
struct CS_LOGIN : public PACKET_HEADER
{
	CS_LOGIN() : PACKET_HEADER(PACKET_LOGIN)
	{
		wPacketSize = sizeof(CS_LOGIN);
	}
};

////////////////////////////
struct SC_LOGIN : public PACKET_HEADER
{
	SC_LOGIN() : PACKET_HEADER(PACKET_LOGIN)
	{
	}
};

////////////////////////////
struct CS_TEST : public PACKET_HEADER
{
	CS_TEST() : PACKET_HEADER(PACKET_TEST)
		, nAccountID(0), nCharacterID(0), nItemID(0)
	{
		wPacketSize = sizeof(CS_TEST);
		printf("CS_TEST packetSize : %d\n", wPacketSize);
	}

	int nAccountID;
	int nCharacterID;
	int nItemID;
};

struct SC_TEST : public PACKET_HEADER
{
	SC_TEST() : PACKET_HEADER(PACKET_TEST)
		, nAccountID(0), nCharacterID(0), nItemID(0)
	{
		wPacketSize = sizeof(SC_TEST);
		printf("CS_TEST packetSize : %d\n", wPacketSize);
	}

	int nAccountID;
	int nCharacterID;
	int nItemID;
};

#pragma pack( pop )