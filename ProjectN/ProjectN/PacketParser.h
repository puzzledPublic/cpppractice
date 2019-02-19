#pragma once

#include "Packet.h"

class PacketParser {
public:
	static PacketType getPacketType(const char* buf);
};