#define MAX_CONNECTIONS 200
#include <iostream>
#include "server.h"
#include "ServerMessageHandler.h"


Server::Server() {
    srand(time(0));
    connectedUsers = std::map<SOCKET, ClientUser*>();
    roomContainers = std::map<std::string, RoomContainer*>();
    initialized = false;
    this->maxConnections = MAX_CONNECTIONS;
    serverMessageHandler = new ServerMessageHandler(this);
}
Server::~Server() {
    delete(serverMessageHandler);
    delete(serverConfig);
}
bool Server::Initialize(ServerConfig* _serverConfig) {
    if (initialized)
        return false;
    
    serverConfig = _serverConfig;

    for (const Room& room : serverConfig->rooms())
    {
        Room* roomData = new Room();
        roomData->CopyFrom(room);
        RoomContainer* container = new RoomContainer(roomData);

        roomContainers.insert({ roomData->name(), container });
    }

    FD_ZERO(&writeSet);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed %d \n", WSAGetLastError());

        WSACleanup();
        return false;
    }

    serverAddr = {};
    serverAddr.sin_family = AF_INET;
    if (serverConfig->address().empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
    }
    else {
        in_addr addr;

        if (inet_pton(AF_INET, serverConfig->address().c_str(), &addr) == -1)
        {
            printf("Invalid IPv4 address, setting as local \n");

            serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
        }
        else {
            serverAddr.sin_addr = addr;
        }

    }
    serverAddr.sin_port = htons(serverConfig->port());

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed %d \n", WSAGetLastError());

        closesocket(serverSocket);
        return false;
    }

    printf("Server listening on port %d \n", serverConfig->port());

    if (listen(serverSocket, maxConnections) == SOCKET_ERROR) {
        printf("Listen failed: %d \n", WSAGetLastError());
        closesocket(serverSocket);
        return false;
    }



    initialized = true;
    fdmax = serverSocket;

    return true;
}

bool Server::IsInitialized()
{
    return initialized;
}

void Server::Run() {
    if (!initialized)
        return;



    while (true)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        for(auto soc : connectedUsers)
        {
            FD_SET(soc.first, &readSet);
            FD_SET(soc.first, &writeSet);
        }

        //always include server, so incoming 'connect()' would be handled properly
        FD_SET(serverSocket, &readSet);
        FD_SET(serverSocket, &writeSet);


        //nfds is beign ignored on winsocks, that's why i pass 0 
        //i have no plans on testing it on linux
        if (select(0, &readSet, &writeSet, NULL, NULL) == SOCKET_ERROR)
        {
            std::cerr << "select " << WSAGetLastError() << "\n";
            exit(1);
        }

        for (u_int i = 0; i < readSet.fd_count; i++)
        {
            SOCKET senderSocket = readSet.fd_array[i];

            //handle connection to server
            if (senderSocket == serverSocket)
            {
                addrlen = sizeof(clientAddr);

                if ((newfd = accept(serverSocket, (sockaddr*)&clientAddr, &addrlen)) == INVALID_SOCKET)
                {
                    printf("accept %d \n", WSAGetLastError());
                }
                else
                {
                    ClientUser* newUser = new ClientUser();
                    newUser->set_id(-1);
                    newUser->set_name("UnknownUser");
                    newUser->set_connectedroomid(-1);
                    connectedUsers.insert({ newfd, newUser });
                    fdmax = (newfd > fdmax) ? newfd : fdmax;

                    char ip[INET_ADDRSTRLEN];
                    printf("selectserver: new connection from %s on "
                        "socket %d\n", inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)), newfd);
                }
            }
            //handle data from client
            else
            {
                if (connectedUsers.contains(senderSocket))
                {
                    if ((receivedBytes = recv(senderSocket, messageBuffer, sizeof(messageBuffer), 0)) <= 0)
                    {
                        if (receivedBytes == 0)
                        {
                            //no connection
                            printf("selectserver: socket %d hung up\n", senderSocket);
                        }
                        else {
                            printf("recv %d \n", WSAGetLastError());
                        }
                        closesocket(senderSocket);
                        connectedUsers.erase(senderSocket);
                    }
                    else
                    {
                        Envelope envelope{};
                        envelope.ParseFromString(messageBuffer);
                        serverMessageHandler->HandleMessage(envelope, senderSocket);
                    }
                }
            }
        }
    }
}

void Server::Stop() {
    if (!initialized)
        return;
}
void Server::Send(const Envelope& envelope, const MessageSendType msgSendType, SOCKET senderSocket)
{
    std::string envelopeParsed;
    envelope.SerializeToString(&envelopeParsed);
    if (msgSendType == MessageSendType::LOCAL)
    {
        if (send(senderSocket, envelopeParsed.data(), envelopeParsed.size(), 0) == SOCKET_ERROR)
        {
            std::cerr << "send " << WSAGetLastError() << "\n";
        }
    }
    else if (msgSendType == MessageSendType::WITHIN_ROOM)
    {
        auto userSender = GetUser(senderSocket);
        for (int j = 0; j < writeSet.fd_count; j++)
        {
            SOCKET dest = writeSet.fd_array[j];

            //send to all except server and sender
            if (dest != serverSocket)
            {
                
                auto userSendTo = GetUser(dest);
                if (userSender->connectedroomid() == userSendTo->connectedroomid())
                {
                    if (send(dest, envelopeParsed.data(), envelopeParsed.size(), 0) == -1)
                    {
                        printf("send %d \n", WSAGetLastError());
                    }
                }
            }
        }
    }
    else if (msgSendType == MessageSendType::WITHIN_ROOM_EXCEPT_THIS)
    {
        auto userSender = GetUser(senderSocket);
        for (int j = 0; j < writeSet.fd_count; j++)
        {
            SOCKET dest = writeSet.fd_array[j];

            //send to all except server and sender
            if (dest != serverSocket && dest != senderSocket)
            {
                auto userSendTo = GetUser(dest);
                if (userSender->connectedroomid() == userSendTo->connectedroomid())
                {
                    if (send(dest, envelopeParsed.data(), envelopeParsed.size(), 0) == -1)
                    {
                        printf("send %d \n", WSAGetLastError());
                    }
                }
            }
        }
    }
    else if (msgSendType == MessageSendType::GLOBAL)
    {
        for (int j = 0; j < writeSet.fd_count; j++)
        {
            SOCKET dest = writeSet.fd_array[j];

            //send to all except server and sender
            if (dest != serverSocket)
            {
                if (send(dest, envelopeParsed.data(), envelopeParsed.size(), 0) == -1)
                {
                    printf("send %d \n", WSAGetLastError());
                }
            }
        }
    }
}
int Server::GetNewUserIdentifier() {
    int uniqueID = (rand() % 10000000);
    printf("uniqueID: %d", uniqueID);
    while (true) {
        for (auto u : connectedUsers)
        {
            if (u.second->id() == uniqueID)
            {
                uniqueID = (rand() % 1000000);
                printf("new uniqueID: %d", uniqueID);
                continue;
            }
        }
        break;
    }

    return uniqueID;
}

ClientUser* Server::GetUser(SOCKET socket) {
    return connectedUsers[socket];
}
bool Server::HasRoom(std::string roomName) {
    return roomContainers.contains(roomName);
}
RoomContainer* Server::GetRoomContainer(std::string roomName) {
    return roomContainers[roomName];
}
RoomContainer* Server::GetRoomContainer(int roomID) {
    for (auto& k : roomContainers)
    {
        auto roomContainer = k.second;
        if (roomContainer->room->id() == roomID)
        {
            return roomContainer;
        }
    }

    return nullptr;
}
std::vector<RoomContainer*> Server::GetRoomContainers()
{
    std::vector<RoomContainer*> _roomContainers = std::vector<RoomContainer*>();

    for (auto& k : roomContainers)
    {
        _roomContainers.push_back(k.second);
    }

    return _roomContainers;
}