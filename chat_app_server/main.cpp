#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <string>
#include <iostream>
#include "server.h"
#include <google/protobuf/util/json_util.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma comment(lib, "ws2_32.lib")

std::shared_ptr<spdlog::logger> file_logger;
Server* server;

int main()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y_%m_%d_%H_%M_%S");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/logs_" + ss.str() + "_" + std::to_string(getpid()) + +".log", 1024 * 1024, 3));
    file_logger = std::make_shared<spdlog::logger>("combined_logger", begin(sinks), end(sinks));
    file_logger->flush_on(spdlog::level::level_enum::info);

    spdlog::set_default_logger(file_logger);

    SPDLOG_LOGGER_INFO(file_logger, "-----------------------------------PROCESS START-----------------------------------");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        SPDLOG_LOGGER_ERROR(file_logger, "WSAStartup failed.\n");
        return 1;
    }

    std::string json_config_file_path = "config.json";
    std::ifstream config_file(json_config_file_path);
    if (!config_file.is_open()) {
        SPDLOG_LOGGER_ERROR(file_logger, "Error: Could not open config file: " + json_config_file_path);
        return 1;
    }

    std::string json_content((std::istreambuf_iterator<char>(config_file)),
        std::istreambuf_iterator<char>());
    config_file.close();

    AdvancedServerConfig* serverConfig = new AdvancedServerConfig();
    google::protobuf::util::JsonStringToMessage(json_content, serverConfig);

    std::string roomsInfo;
    roomsInfo += "\n--- Server Configuration ---\n";
    roomsInfo += "Server Address: " + serverConfig->server().address() + "\n";
    roomsInfo += "Server Port: " + std::to_string(serverConfig->server().port()) + "\n";

    roomsInfo += "--- Room Configuration ---\n";
    for (const auto& room : serverConfig->rooms()) {
        roomsInfo += std::format("  ID: {}, Name: {}, Has Password: {}, Password: {}, MaxConnections: {} IsPublic:{}\n", room.id(), room.name(), (room.haspassword() ? "true" : "false"), room.password(), room.maxconnections(), (room.ispublic() ? "true" : "false"));

    }
    SPDLOG_LOGGER_INFO(file_logger, roomsInfo);

    server = new Server(file_logger);
    
    server->Initialize(serverConfig);  //localhost


    server->Run();


    SPDLOG_LOGGER_INFO(file_logger, "-----------------------------------PROCESS END-----------------------------------");
}