#pragma once

//protobuf
#include "communication.pb.h"

class ClientChatMessage
{
public:
	ClientChatMessage() = default;
	time_t receiveTime;
	ChatMessage chatMessage;
};
