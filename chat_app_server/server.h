#pragma once
#include <string>
#include <ws2tcpip.h>

class Server
{
public:
	Server(const int& maxConnections);
	bool Initialize(const std::string& serverName, const int& serverPort);
	bool IsInitialized();
	void Run();
	void Stop();
protected:
	bool initialized;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	int maxConnections;

	fd_set master;
	fd_set read_fds;
	sockaddr_in clientAddr;
	SOCKET fdmax;
	SOCKET newfd;
	char messageBuffer[256];
	int receivedBytes;
	int addrlen;
};