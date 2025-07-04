#pragma once
#include "communication.pb.h"
#include <ftxui/dom/elements.hpp>  // for Elements, gridbox, Fit, operator|, text, border, Element
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/node.hpp"      // for Render
#include "ftxui/screen/color.hpp"  // for ftxui
#include "ftxui/component/component.hpp"

#include <stdio.h>  // for getchar
#include <memory>                  
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <random>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#pragma comment(lib, "ws2_32.lib")


#define LOGGING true
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080
using namespace ftxui;

SOCKET clientSocket;
std::string currentInput;
ClientUser clientUser;
std::vector<ChatMessage> chatHistory;

std::mutex chatMutex;
std::thread uiThread;
std::thread syncChatThread;
std::shared_ptr<spdlog::logger> file_logger;

void EndProcess()
{
	closesocket(clientSocket);
	WSACleanup();
	file_logger->info("-----------------------------------PROCESS END-----------------------------------");
}

ChatMessage GetChatMessage( std::string msg,  std::string sender)
{
	ChatMessage chatMessage;
	chatMessage.set_message(msg);
	chatMessage.set_sender(sender);

	return chatMessage;
}

void AddChatMessage(ChatMessage message)
{
	chatMutex.lock();
	chatHistory.push_back(message);
	chatMutex.unlock();
}

bool SendToServer(Envelope envelope) {
	file_logger->info("Send To Server");
	std::string serializedEnvelope = envelope.SerializeAsString();
	int sendResult = send(clientSocket, serializedEnvelope.data(), serializedEnvelope.size(), 0);

	if (sendResult == SOCKET_ERROR) {
		if (LOGGING)
		{
			file_logger->error("Send to Server Failed error {}", WSAGetLastError());
		}
		EndProcess();
		return false;
	}

	file_logger->info("Send Finish");
	return true;
}

void HandleInput() {
	if (currentInput.empty())
		return;

	if (currentInput[0] == '/') // command
	{
		file_logger->info("Sending command");
		auto msg = GetChatMessage(currentInput, "[ONLY YOU]");
		AddChatMessage(msg);
		
		std::istringstream iss(currentInput);
		std::vector<std::string> words;
		std::string word;
		// The extraction operator (>>) reads whitespace-separated words
		while (iss >> word) {
			words.push_back(word);
		}

		Envelope envelope{};

		envelope.set_type(MessageType::COMMAND);
		CommandRequest cr{};
		cr.set_request(words[0]);

		for (size_t i = 1; i < words.size(); ++i) {
			cr.add_requestparameters(words[i]);
		}

		envelope.set_payload(cr.SerializeAsString());
		SendToServer(envelope);
	}
	else if(clientUser.id() != -1 && clientUser.connectedroomid() != -1)
	{
		file_logger->info("Send Chat Message");
		auto msg = GetChatMessage(currentInput, clientUser.name());

		Envelope envelope{};
		envelope.set_type(MessageType::CHAT_MESSAGE);
		envelope.set_payload(msg.SerializeAsString());

		if (SendToServer(envelope))
		{
			AddChatMessage(msg);
		}
	}
	else 
	{
		file_logger->warn("Cannot send chat message");
		auto msg = GetChatMessage("You need to set nickname and join any room to send messages.", "[ONLY YOU]");
		AddChatMessage(msg);
	}


	currentInput.clear();
}

void UpdateChat() {
	auto message_component = Container::Vertical({});


	auto input_component = Input(&currentInput, "Type your message here");


	auto main_component = Container::Vertical({
		message_component,
		input_component
		});

	auto main_renderer = Renderer(main_component, [&] {
		Elements message_elements;

		chatMutex.lock();
		for (const auto& msg : chatHistory) {
			message_elements.push_back(paragraph(msg.sender() + ":" + msg.message()));
		}
		chatMutex.unlock();

		return vbox({
			vbox(message_elements) | flex,
			separator(),
			hbox({ input_component->Render() }) | border
			}) | border | flex;
		});

	auto component = CatchEvent(main_renderer, [&](Event event) {
		if (event == Event::Return) {
			HandleInput();
			return true;
		}
		return false;
		});

	 auto screen = ScreenInteractive::Fullscreen();

	 std::thread autoRefreshScreen([&screen] {
		 while (true)
		 {
			 using namespace std::chrono_literals;
			 screen.PostEvent(Event::Custom);
			 std::this_thread::sleep_for(0.2s);
		 }
	 });

	 screen.Loop(component);

}

void ListenForMessages(SOCKET clientSocket)
{

	while (true) {
		char buffer[256];
		int receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (receivedBytes <= 0) {
			if (receivedBytes == 0) {
				if (LOGGING)
				{
					file_logger->error("Server disconnected");
				}
				EndProcess();
				break;
			}
		}
		else {

			file_logger->info("Message from server received");

			Envelope envelope;
			envelope.ParseFromString(buffer);

			switch (envelope.type())
			{
			case MessageType::CHAT_MESSAGE: 
			{
				file_logger->info("Received {}", std::to_string(MessageType::CHAT_MESSAGE));
				ChatMessage chatMessage;
				chatMessage.ParseFromString(envelope.payload());
				AddChatMessage(chatMessage);
				break;
			}
			case MessageType::USER_JOIN_ROOM:
			case MessageType::USER_LEAVE_ROOM: 
			{
				file_logger->info("Received {} or {}", std::to_string(MessageType::USER_JOIN_ROOM), std::to_string(MessageType::USER_LEAVE_ROOM));


				ChatMessage chatMessage = GetChatMessage(envelope.payload(), "[EVERYONE]");
				AddChatMessage(chatMessage);
				break;
			}
			case MessageType::COMMAND: 
			{
				file_logger->info("Received command response");

				CommandResponse cres;
				cres.ParseFromString(envelope.payload());
				switch (cres.type())
				{
				case CommandType::HELP:
				case CommandType::ROOM_LIST:
				case CommandType::INVALID: 
				{
					file_logger->info("Received {} or {} or {}", std::to_string(CommandType::HELP), std::to_string(CommandType::ROOM_LIST), std::to_string(CommandType::INVALID));

					ChatMessage chatMessage = GetChatMessage(cres.response(), "[ONLY YOU]");
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::NICKNAME: 
				{
					file_logger->info("Received {}", std::to_string(CommandType::NICKNAME));

					clientUser.ParseFromString(cres.response());

					ChatMessage chatMessage = GetChatMessage("New nickname " + clientUser.name(), "[ONLY YOU]");
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::JOIN_ROOM:
				{
					file_logger->info("Received {}", std::to_string(CommandType::JOIN_ROOM));

					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Connected to the channel.", "[ONLY YOU]");
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::LEAVE_ROOM:
				{
					file_logger->info("Received {}", std::to_string(CommandType::LEAVE_ROOM));

					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Disconnected from the channel.", "[ONLY YOU]");
					AddChatMessage(chatMessage);

					break;
				}
				default:
					break;
				}
			}
			default:
				break;
			}
		}
	}
}



int main()
{
	file_logger = spdlog::rotating_logger_mt("file_logger", "logs/logs.log", 1024 * 1024, 3);
	file_logger->flush_on(spdlog::level::level_enum::info);
	spdlog::set_default_logger(file_logger);

	file_logger->info("-----------------------------------PROCESS START-----------------------------------");


	uiThread = std::thread(UpdateChat);
	uiThread.detach();

	WSADATA wsaData;

	ChatMessage chatMessage;

	//create default user, its ID will be adjusted on /setnick command and room on /joinRoom or /leaveRoom
	clientUser = ClientUser{};
	clientUser.set_name("UserUnknown");
	clientUser.set_id(-1);
	clientUser.set_connectedroomid(-1);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		if (LOGGING)
		{
			file_logger->error("WSAStartup failed");
		}
		return 1;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		if (LOGGING)
		{
			file_logger->error("Socket creation failed: {}", WSAGetLastError());
		}
		WSACleanup();
		return 1;
	}


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(SERVER_PORT);

	if (LOGGING)
	{
		file_logger->info("Trying to connect to server. address:{} port:{}", SERVER_ADDRESS, SERVER_PORT);
	}

	int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	if (returnCall == SOCKET_ERROR)
	{
		if (LOGGING)
		{
			file_logger->error("Connection error return call: {} socketVal: {}", returnCall, clientSocket);
		}
		EndProcess();
		return 1;
	}

	if (LOGGING) 
	{
		file_logger->info("Connection established! address:{} port: {}", SERVER_ADDRESS, SERVER_PORT);
	}
	chatMessage = GetChatMessage("/help to see commands", "[ONLY YOU]");
	AddChatMessage(chatMessage);



	syncChatThread = std::thread(ListenForMessages, clientSocket);
	syncChatThread.join();

	return 1;
}


