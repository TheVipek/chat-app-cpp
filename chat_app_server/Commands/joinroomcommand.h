#pragma once
#include "command.h"

class JoinRoomCommand : public Command
{
public:
	JoinRoomCommand() : Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
