#include <iostream>
#include "server.h"

Server::Server(const int& maxConnections) {

    connectedSockets = std::list<SOCKET>();
    initialized = false;
    this->maxConnections = maxConnections;
}

bool Server::Initialize(const std::string& serverAddress, const int& serverPort) {
    if (initialized)
        return false;
    
    FD_ZERO(&writeSet);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
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
            std::cerr << "Invalid IPv4 address, setting as local \n";
            serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
        }
        else {
            serverAddr.sin_addr = addr;
        }

    }
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        return false;
    }

    std::cout << "Server listening on port" << serverPort << "...\n";

    if (listen(serverSocket, maxConnections) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        return false;
    }

    initialized = true;

    connectedSockets.push_back(serverSocket);
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

        for(auto soc : connectedSockets)
        {
            FD_SET(soc, &readSet);
            FD_SET(soc, &writeSet);
        }

        

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
                    std::cerr << "accept " << WSAGetLastError() << "\n";
                }
                else
                {
                    connectedSockets.push_back(newfd);
                    fdmax = (newfd > fdmax) ? newfd : fdmax;

                    char ip[INET_ADDRSTRLEN];
                    printf("selectserver: new connection from %s on "
                        "socket %d\n", inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)), newfd);
                }
            }
            //handle data from client
            else
            {
                if ((receivedBytes = recv(senderSocket, messageBuffer, sizeof(messageBuffer), 0)) <= 0)
                {
                    if (receivedBytes == 0)
                    {
                        //no connection
                        printf("selectserver: socket %d hung up\n", senderSocket);
                    }
                    else {
                        std::cerr << "recv " << WSAGetLastError() << "\n";
                    }
                    closesocket(senderSocket);
                    connectedSockets.remove(senderSocket);
                }
                else
                {
                    //no specific handling for Envelope for now

                    for (int j = 0; j < writeSet.fd_count; j++)
                    {
                        SOCKET dest = writeSet.fd_array[j];

                        //send to all except server and sender
                        if (dest != serverSocket && dest != senderSocket)
                        {
                            if (send(dest, messageBuffer, receivedBytes, 0) == -1)
                            {
                                std::cerr << "send " << WSAGetLastError() << "\n";
                            }
                        }
                    }
                }
                

            }
        }
    }
}

//void Server::HandleEnvelope(const Envelope& envelope, const SOCKET& recvFromSocket)
//{
//    switch (envelope.type())
//    {
//    case MessageType::CHAT_MESSAGE:
//        for (int j = 0; j <= readSet.fd_count; j++)
//        {
//            SOCKET dest = readSet.fd_array[j];
//            if (FD_ISSET(dest, &writeSet))
//            {
//                //send to all except server
//                if (dest != serverSocket && dest != recvFromSocket)
//                {
//                    if (send(dest, messageBuffer, receivedBytes, 0) == -1)
//                    {
//                        std::cerr << "send " << WSAGetLastError() << "\n";
//                    }
//                }
//            }
//        }
//        break;
//    default:
//        break;
//    }
//}


void Server::Stop() {
    if (!initialized)
        return;
}