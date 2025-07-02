#pragma once
#include <string>
#include <ws2tcpip.h>
#include "../generated/communication.pb.h"
#include <stdlib.h>
#include <time.h>

class ServerMessageHandler;

class Server
{
public:
	Server(const int& maxConnections);
	~Server();
	friend ServerMessageHandler;
	bool Initialize(const std::string& serverName, const int& serverPort);
	bool IsInitialized();
	void Run();
	void Stop();
protected:
	bool initialized;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	int maxConnections;
	
	ServerMessageHandler* serverMessageHandler;
	std::map<SOCKET, ClientUser> connectedUsers;
	fd_set writeSet;
	fd_set readSet;
	sockaddr_in clientAddr;
	SOCKET fdmax;
	SOCKET newfd;
	char messageBuffer[256];
	int receivedBytes;
	int addrlen;
};