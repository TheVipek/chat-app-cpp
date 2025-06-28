#pragma once
#include <string>
#include <ws2tcpip.h>

class Server
{
public:
	Server(const std::string& serverName, const int& serverPort, const int& maxConnections);
	bool Initialize(const std::string& serverName, const int& serverPort);
	bool IsInitialized();
	void Run();
	void Stop();
protected:
	bool initialized;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	int maxConnections;
};