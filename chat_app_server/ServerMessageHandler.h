#pragma once
#include "server.h"
#include "Commands/servercommand.h"

class ServerMessageHandler
{
public: 
	ServerMessageHandler(Server* server);
	void HandleMessage(const Envelope& envelope, const SOCKET recvFromSocket);
protected:
	Server* server;
	std::map<std::string, std::unique_ptr<ServerCommand>> commands;
};
