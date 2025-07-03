#pragma once

#include "command.h"

class LeaveRoomCommand : public Command
{
public:
	LeaveRoomCommand() : Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
