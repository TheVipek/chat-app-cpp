#pragma once
#include "server.h"
class ServerMessageHandler
{
public: 
	ServerMessageHandler(Server* server);
	void HandleMessage(const Envelope& envelope, const SOCKET& recvFromSocket);
protected:
	Server* server;
	int GetNewUserIdentifier();
};
