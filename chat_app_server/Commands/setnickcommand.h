#pragma once
#include "servercommand.h"

class SetNickCommand: public ServerCommand
{
public:
	SetNickCommand() : ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
