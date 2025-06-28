#include <iostream>
#include "server.h"

Server::Server(const std::string& serverName, const int& serverPort, const int& maxConnections) {

    initialized = false;
    this->maxConnections = maxConnections;
	Initialize(serverName, serverPort);
}

bool Server::Initialize(const std::string& serverName, const int& serverPort) {
    if (initialized)
        return false;
    
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }

    serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = serverName.empty() ? INADDR_ANY : inet_addr(serverName.c_str());
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
        // Accept a client
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
            
        }
    }
}

void Server::Stop() {
    if (!initialized)
        return;
}