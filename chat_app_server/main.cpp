#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <iostream>
#include "server.h"
#include <google/protobuf/util/json_util.h>
#pragma comment(lib, "ws2_32.lib")

Server* server;

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    std::string json_config_file_path = "config.json";
    std::ifstream config_file(json_config_file_path);
    if (!config_file.is_open()) {
        std::cerr << "Error: Could not open config file: " << json_config_file_path << std::endl;
        return 1;
    }

    std::string json_content((std::istreambuf_iterator<char>(config_file)),
        std::istreambuf_iterator<char>());
    config_file.close();

    ServerConfig* serverConfig = new ServerConfig();
    google::protobuf::util::JsonStringToMessage(json_content, serverConfig);

    std::cout << "--- Server Configuration ---" << std::endl;
    std::cout << "Server Address: " << serverConfig->address() << std::endl;
    std::cout << "Server Port: " << serverConfig->port() << std::endl;

    std::cout << "\n--- Room Configuration ---" << std::endl;
    for (const auto& room : serverConfig->rooms()) {
        std::cout << "  ID: " << room.id()
            << ", Name: " << room.name()
            << ", Has Password: " << (room.haspassword() ? "true" : "false")
            << ", Password: " << room.password()
            << ", MaxConnections: " << room.maxconnections()
            << std::endl;
    }


    server = new Server();
    
    server->Initialize(serverConfig);  //localhost


    server->Run();

}