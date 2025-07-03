#pragma once
#include <string>
#include <ws2tcpip.h>
#include "../generated/communication.pb.h"
#include <stdlib.h>
#include <time.h>
#include "Room/RoomContainer.h"
class ServerMessageHandler;

class Server
{
public:
	Server();
	~Server();
	friend ServerMessageHandler;
	bool Initialize(ServerConfig* serverConfig);
	bool IsInitialized();
	void Run();
	void Stop();
	void Send(const Envelope& envelope, const MessageSendType msgSendType, SOCKET senderSocket);
	int GetNewUserIdentifier();
	bool HasRoom(std::string roomName);
	RoomContainer* GetRoomContainer(std::string roomName);
	RoomContainer* GetRoomContainer(int roomID);
	std::vector<RoomContainer*> GetRoomContainers();
	ClientUser* GetUser(SOCKET socket);
protected:
	bool initialized;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	int maxConnections;
	
	ServerConfig* serverConfig;
	ServerMessageHandler* serverMessageHandler;
	std::map<SOCKET, ClientUser*> connectedUsers;
	std::map<std::string, RoomContainer*> roomContainers;
	fd_set writeSet;
	fd_set readSet;
	sockaddr_in clientAddr;
	SOCKET fdmax;
	SOCKET newfd;
	char messageBuffer[256];
	int receivedBytes;
	int addrlen;
};