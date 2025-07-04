#pragma once
#include "servercommand.h"

class JoinRoomCommand : public ServerCommand
{
public:
	JoinRoomCommand(std::shared_ptr<spdlog::logger> _file_logger) : ServerCommand(_file_logger) {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
