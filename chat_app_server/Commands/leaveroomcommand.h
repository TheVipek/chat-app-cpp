#pragma once

#include "servercommand.h"

class LeaveRoomCommand : public ServerCommand
{
public:
	LeaveRoomCommand() : ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
