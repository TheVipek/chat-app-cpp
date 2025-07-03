#pragma once
#include <ws2tcpip.h>
#include "../../generated/communication.pb.h"
#include "../server.h"

class Command
{
public: 
	Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) = 0;
};