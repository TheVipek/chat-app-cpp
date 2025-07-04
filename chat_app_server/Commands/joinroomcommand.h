#pragma once
#include "servercommand.h"

class JoinRoomCommand : public ServerCommand
{
public:
	JoinRoomCommand() : ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
