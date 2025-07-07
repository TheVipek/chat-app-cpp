#pragma once
#include "servercommand.h"

class CreateRoom : public ServerCommand
{
public:
	CreateRoom(std::shared_ptr<spdlog::logger> _file_logger) : ServerCommand(_file_logger) {};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) override;
};
