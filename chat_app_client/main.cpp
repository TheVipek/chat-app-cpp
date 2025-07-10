#pragma once

//protobuf
#include "communication.pb.h"
#include <google/protobuf/util/json_util.h>

#include "chatui.h"
#include "chatlogic.h"

//spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

//Windows
#include <winsock2.h>
#include <ws2tcpip.h>

#include <fstream>
#include <string> 
#include <memory>

#include "helpers.h"
#include "message_mappings.h"

#pragma comment(lib, "ws2_32.lib")

using namespace ChatApp;

std::thread uiThread;
std::thread syncChatThread;
std::shared_ptr<spdlog::logger> file_logger;

std::shared_ptr<ChatUI> chatUI;
std::shared_ptr<ChatLogic> chatLogic;




void EndProcess()
{
	WSACleanup();
	SPDLOG_LOGGER_INFO(file_logger, "-----------------------------------PROCESS END-----------------------------------");
	exit(1);
}


int main()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);



	//log with unique file name
	std::stringstream ss;    
	ss << std::put_time(std::localtime(&in_time_t), "%Y_%m_%d_%H_%M_%S");
	file_logger = spdlog::rotating_logger_mt("file_logger", "logs/logs_" + ss.str() + "_" + std::to_string(getpid()) + +".log", 1024 * 1024, 3);
	file_logger->flush_on(spdlog::level::level_enum::info);
	spdlog::set_default_logger(file_logger);
	SPDLOG_LOGGER_INFO(file_logger,"-----------------------------------PROCESS START-----------------------------------");

	std::string json_config_file_path = "config.json";
	std::ifstream config_file(json_config_file_path);
	if (!config_file.is_open()) {
		
		SPDLOG_LOGGER_ERROR(file_logger ,"Error: Could not open config file: " + json_config_file_path);
		return 1;
	}

	std::string json_content((std::istreambuf_iterator<char>(config_file)),
		std::istreambuf_iterator<char>());
	config_file.close();

	SimpleServerConfig* serverConfig = new SimpleServerConfig();
	google::protobuf::util::JsonStringToMessage(json_content, serverConfig);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		SPDLOG_LOGGER_ERROR(file_logger, "WSAStartup failed");
		return 0;
	}
	
	try
	{
		chatLogic = std::make_shared<ChatLogic>(file_logger);
		chatUI = std::make_shared<ChatUI>(file_logger, chatLogic);

		uiThread = std::thread([]() {
			chatUI->Update();
			});
		uiThread.detach();

		auto clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (clientSocket == INVALID_SOCKET)
		{
			SPDLOG_LOGGER_ERROR(file_logger, "Socket creation failed: {}", WSAGetLastError());

			WSACleanup();
			return 0;
		}


		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		inet_pton(AF_INET, serverConfig->address().c_str(), &serverAddr.sin_addr);
		serverAddr.sin_port = htons(serverConfig->port());

		SPDLOG_LOGGER_INFO(file_logger, "Trying to connect to server. address:{} port:{}", serverConfig->address(), serverConfig->port());


		int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

		if (returnCall == SOCKET_ERROR)
		{
			SPDLOG_LOGGER_ERROR(file_logger, "Connection error return call: {} socketVal: {}", returnCall, clientSocket);
			EndProcess();
			return 0;
		}

		SPDLOG_LOGGER_INFO(file_logger, "Connection established! address:{} port: {}", serverConfig->address(), serverConfig->port());

		chatLogic->Initialize(clientSocket);

		ChatMessage chatMessage;
		chatMessage = chatLogic->GetChatMessage("/help to see commands", GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
		chatLogic->AddChatMessage(chatMessage);


		syncChatThread = std::thread([]() {
			chatLogic->ListenForMessages();
			});
		syncChatThread.join();

		
	}
	catch(...)
	{
		SPDLOG_LOGGER_CRITICAL(file_logger, "Client crashed.");
		EndProcess();
	}
}


