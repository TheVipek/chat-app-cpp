#include <iostream>
#include "server.h"
#include "ServerMessageHandler.h"

Server::Server(const int& maxConnections) {
    srand(time(0));
    connectedUsers = std::map<SOCKET, ClientUser>();
    initialized = false;
    this->maxConnections = maxConnections;
    serverMessageHandler = new ServerMessageHandler(this);
}
Server::~Server() {
    delete(serverMessageHandler);
}
bool Server::Initialize(const std::string& serverAddress, const int& serverPort) {
    if (initialized)
        return false;
    
    FD_ZERO(&writeSet);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed %d \n", WSAGetLastError());

        WSACleanup();
        return false;
    }

    serverAddr = {};
    serverAddr.sin_family = AF_INET;
    if (serverAddress.empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
    }
    else {
        in_addr addr;

        if (inet_pton(AF_INET, serverAddress.c_str(), &addr) == -1)
        {
            printf("Invalid IPv4 address, setting as local \n");

            serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
        }
        else {
            serverAddr.sin_addr = addr;
        }

    }
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed %d \n", WSAGetLastError());

        closesocket(serverSocket);
        return false;
    }

    printf("Server listening on port %d \n", serverPort);

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
                    ClientUser serverUser{};
                    serverUser.set_id(-1);
                    serverUser.set_name("UnknownUser");
                    connectedUsers.insert({ newfd, serverUser });
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

