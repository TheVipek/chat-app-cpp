#pragma once
#include <string>
#include <ws2tcpip.h>
#include "../generated/communication.pb.h"
#include <stdlib.h>
#include <time.h>
#include "Room/RoomContainer.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

class ServerMessageHandler;

class Server
{
public:
	Server(std::shared_ptr<spdlog::logger> _file_logger);
	~Server();
	friend ServerMessageHandler;
	bool Initialize(ServerConfig* serverConfig);
	bool IsInitialized();
	void Run();
	void Stop();
	std::vector<SOCKET> Send(const Envelope& envelope, const MessageSendType msgSendType, SOCKET senderSocket);
	int GetNewUserIdentifier();
	bool HasRoom(std::string roomName);
	RoomContainer* GetRoomContainer(std::string roomName);
	RoomContainer* GetRoomContainer(int roomID);
	std::vector<RoomContainer*> GetRoomContainers();
	ClientUser* GetUser(SOCKET socket);
protected:
	std::shared_ptr<spdlog::logger> file_logger;
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

	std::unordered_map<SOCKET, std::chrono::steady_clock::time_point> lastPingTime;
	int pingTimeoutSeconds = 5;
	void PingClients();
};