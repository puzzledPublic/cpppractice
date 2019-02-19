#pragma once

#pragma pack(push, 1)

struct Packet {
	char header[4];
	char body[256];
};

#pragma pack(pop)

enum class PacketType
{
	STRING = 0,
	PLUS = 1,
	MINUS = 2
};