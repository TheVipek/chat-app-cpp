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
	bool Initialize(AdvancedServerConfig* serverConfig);
	bool IsInitialized();
	void Run();
	void Stop();
	std::vector<SOCKET> Send(Envelope envelope, SOCKET senderSocket);
	/// <summary>
	/// Used for DIRECT MessageSendType, otherwise you should use normal Send method.
	/// </summary>
	/// <param name="envelope"></param>
	/// <param name="senderSocket"></param>
	/// <param name="targetSocket"></param>
	/// <returns></returns>
	std::vector<SOCKET> SendDirect(Envelope envelope, SOCKET targetSockets[], int count);
	int GetNewUserIdentifier();
	bool HasRoom(std::string roomName);
	int MaxRoomID();
	RoomContainer* GetRoomContainer(std::string roomName);
	RoomContainer* GetRoomContainer(int roomID);
	RoomContainer* CreateRoom(std::string name, int maxConnections, bool isPublic, std::string password, bool destroyOnEmpty);
	void RemoveRoom(std::string name);
	std::vector<RoomContainer*> GetRoomContainers();
	bool UserExists(SOCKET socket);
	bool UserExists(int userID);
	ClientUser* GetUser(SOCKET socket);
	ClientUser* GetUser(int userID);
	std::vector<ClientUser> GetAllUsersTemp();
	SOCKET GetUserSocket(ClientUser* client);

	/// <summary>
	/// Adds empty user, by default client data is not propagated fully, until /setnick command call.
	/// </summary>
	/// <param name="sock"></param>
	/// <param name="id"></param>
	/// <param name="name"></param>
	/// <param name="roomID"></param>
	void AddStartUpUser(SOCKET sock);
	/// <summary>
	/// Updates existing user data 
	/// </summary>
	/// <param name="sock"></param>
	/// <param name="id">If -1 is passed, it won't be changed</param>
	/// <param name="name">If empty string is passed, it won't be changed</param>
	/// <param name="roomID">It won't be changed only if passed roomID is the same.</param>
	void UpdateExistingUserData(SOCKET sock, int id, std::string name, int roomID, std::string roomName);
	/// <summary>
	/// Removes user from all collections, closes its socket and free from memory
	/// </summary>
	/// <param name="socket"></param>
	/// <param name="notify">If true and user is currently in room, it will send notification to the other clients</param>
	void RemoveUser(SOCKET socket, bool notify);

	void SendUpdateOfAllUsers();
protected:
	std::shared_ptr<spdlog::logger> file_logger;
	bool initialized;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	int maxConnections;
	
	AdvancedServerConfig* serverConfig;
	ServerMessageHandler* serverMessageHandler;
	std::map<SOCKET, ClientUser*> connectedUsers;
	std::unordered_map<int, ClientUser*> userByID;
	std::map<std::string, RoomContainer*> roomContainers;
	std::unordered_map<SOCKET, std::chrono::steady_clock::time_point> lastTimeActivity;
	fd_set writeSet;
	fd_set readSet;
	sockaddr_in clientAddr;
	SOCKET fdmax;
	SOCKET newfd;
	
	int receivedBytes;
	int addrlen;

	int pingTimeoutSeconds = 5;
	void PingClients();
	void UpdateClientActivity(SOCKET sender);
};