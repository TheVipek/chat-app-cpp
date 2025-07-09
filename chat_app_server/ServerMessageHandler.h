#pragma once
#include "server.h"
#include "Commands/servercommand.h"

class ServerMessageHandler
{
public: 
	ServerMessageHandler(Server* server, std::shared_ptr<spdlog::logger> _file_logger);
	~ServerMessageHandler();
	void HandleMessage(const Envelope& envelope, const SOCKET recvFromSocket);
protected:
	std::shared_ptr<spdlog::logger> file_logger;
	Server* server;
	std::map<std::string, std::unique_ptr<ServerCommand>> commands;
};
