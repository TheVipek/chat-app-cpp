#pragma once
#include <ws2tcpip.h>
#include "../../generated/communication.pb.h"
#include "../server.h"

class ServerCommand
{
public: 
	ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) = 0;
};