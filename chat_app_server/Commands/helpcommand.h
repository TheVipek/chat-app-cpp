#pragma once
#include "command.h"

class HelpCommand : public Command
{
public:
	HelpCommand() : Command() {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};