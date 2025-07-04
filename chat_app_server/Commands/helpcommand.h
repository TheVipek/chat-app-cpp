#pragma once
#include "servercommand.h"

class HelpCommand : public ServerCommand
{
public:
	HelpCommand() : ServerCommand() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};