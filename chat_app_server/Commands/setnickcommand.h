#pragma once
#include "command.h"

class SetNickCommand: public Command
{
public:
	SetNickCommand() : Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
