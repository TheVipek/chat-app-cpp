#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "server.h"
#pragma comment(lib, "ws2_32.lib")

Server* server;

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    server = new Server();
    

}