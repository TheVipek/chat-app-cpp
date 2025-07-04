#pragma once
#include <ws2tcpip.h>
#include "../../generated/communication.pb.h"
#include "../server.h"

class ServerCommand
{
public: 
	ServerCommand(std::shared_ptr<spdlog::logger> _file_logger) 
	{
		file_logger = _file_logger;
	};
	virtual void Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server) = 0;
protected:
	std::shared_ptr<spdlog::logger> file_logger;

};