#pragma once
#include <string>
#include <ws2tcpip.h>
#include "../generated/communication.pb.h"
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

	std::list<SOCKET> connectedSockets;
	fd_set writeSet;
	fd_set readSet;
	sockaddr_in clientAddr;
	SOCKET fdmax;
	SOCKET newfd;
	char messageBuffer[256];
	int receivedBytes;
	int addrlen;
	void HandleEnvelope(const Envelope& envelope, const SOCKET& recvFromSocket);
};