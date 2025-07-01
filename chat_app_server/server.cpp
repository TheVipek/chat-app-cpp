#include <iostream>
#include "server.h"

Server::Server(const int& maxConnections) {

    initialized = false;
    this->maxConnections = maxConnections;
}

bool Server::Initialize(const std::string& serverAddress, const int& serverPort) {
    if (initialized)
        return false;
    
    FD_ZERO(&master);

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
    FD_SET(serverSocket, &master);
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
        FD_ZERO(&read_fds);
        for (u_int i = 0; i < master.fd_count; i++) {
            FD_SET(master.fd_array[i], &read_fds);
        }
        if (select(fdmax, &read_fds, NULL, NULL, NULL) == SOCKET_ERROR)
        {
            std::cerr << "select " << WSAGetLastError() << "\n";
            exit(1);
        }

        for (u_int i = 0; i < read_fds.fd_count; i++)
        {
            SOCKET sock = read_fds.fd_array[i];
            if (FD_ISSET(sock, &read_fds))
            {
                //handle connection to server
                if (sock == serverSocket)
                {
                    addrlen = sizeof(clientAddr);

                    if ((newfd = accept(serverSocket, (sockaddr*)&clientAddr, &addrlen)) == INVALID_SOCKET)
                    {
                        std::cerr << "accept " << WSAGetLastError() << "\n";
                    }
                    else
                    {
                        FD_SET(newfd, &master);
                        fdmax = (newfd > fdmax) ? newfd : fdmax;

                        char ip[INET_ADDRSTRLEN];
                        printf("selectserver: new connection from %s on "
                            "socket %d\n", inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)), newfd);
                    }
                }
                //handle data from client
                else
                {
                    if ((receivedBytes = recv(sock, messageBuffer, sizeof(messageBuffer), 0)) <= 0)
                    {
                        if (receivedBytes == 0)
                        {
                            //no connection
                            printf("selectserver: socket %d hung up\n", sock);
                        }
                        else {
                            std::cerr << "recv " << WSAGetLastError() << "\n";
                        }
                        closesocket(sock);
                        FD_CLR(sock, &master); // delete from main collection
                    }
                    else
                    {
                        //no specific handling for Envelope for now

                        for (int j = 0; j <= read_fds.fd_count; j++)
                        {
                            SOCKET dest = read_fds.fd_array[j];
                            if (FD_ISSET(dest, &master))
                            {
                                //send to all except server
                                if (dest != serverSocket && dest != sock)
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
    }
}

void Server::HandleEnvelope(const Envelope& envelope, const SOCKET& recvFromSocket)
{
    switch (envelope.type())
    {
    case MessageType::CHAT_MESSAGE:
        for (int j = 0; j <= read_fds.fd_count; j++)
        {
            SOCKET dest = read_fds.fd_array[j];
            if (FD_ISSET(dest, &master))
            {
                //send to all except server
                if (dest != serverSocket && dest != recvFromSocket)
                {
                    if (send(dest, messageBuffer, receivedBytes, 0) == -1)
                    {
                        std::cerr << "send " << WSAGetLastError() << "\n";
                    }
                }
            }
        }
        break;
    default:
        break;
    }
}


void Server::Stop() {
    if (!initialized)
        return;
}