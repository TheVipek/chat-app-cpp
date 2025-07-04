#pragma once

#include "servercommand.h"

class RoomListCommand : public ServerCommand
{
public:
	RoomListCommand() : ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
