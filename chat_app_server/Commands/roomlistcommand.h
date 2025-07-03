#pragma once

#include "command.h"

class RoomListCommand : public Command
{
public:
	RoomListCommand() : Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
