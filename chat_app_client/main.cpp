#pragma once
#include "communication.pb.h"
#include <ftxui/dom/elements.hpp>  // for Elements, gridbox, Fit, operator|, text, border, Element
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/node.hpp"      // for Render
#include "ftxui/screen/color.hpp"  // for ftxui
#include "ftxui/component/component.hpp"
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <string>
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
#include <ctime>;

#pragma comment(lib, "ws2_32.lib")


#define LOGGING true
using namespace ftxui;
class ClientChatMessage
{
public:
	time_t receiveTime;
	ChatMessage chatMessage;
};

SOCKET clientSocket;
std::string currentInput;
ClientUser clientUser;
std::vector<ClientChatMessage> chatHistory;
int pingTimeoutSeconds = 5;
int timeoutTries = 3;
std::chrono::steady_clock::time_point lastPingTime;
std::mutex chatMutex;
std::thread uiThread;
std::thread pingThread;
std::thread syncChatThread;
std::shared_ptr<spdlog::logger> file_logger;
float scroll_y = 1.f;
std::map<MessageSendType, std::string> sendTypeMappings;

std::string GetSendTypeString(MessageSendType messageSendType)
{
	if (sendTypeMappings.contains(messageSendType))
	{
		return sendTypeMappings[messageSendType];
	}

	return "Unknown Send Type";
}

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
	ClientChatMessage clientChatMessage;
	time_t timestamp;
	time(&timestamp);

	clientChatMessage.receiveTime = timestamp;
	clientChatMessage.chatMessage = message;
	chatHistory.push_back(clientChatMessage);
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
		auto msg = GetChatMessage(currentInput, GetSendTypeString(MessageSendType::LOCAL));
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
		std::string name = std::format("{}#{}", clientUser.name(), clientUser.id());
		auto msg = GetChatMessage(currentInput, name);

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
		auto msg = GetChatMessage("You need to set nickname and join any room to send messages.", GetSendTypeString(MessageSendType::LOCAL));
		AddChatMessage(msg);
	}


	currentInput.clear();
}

void UpdateChat() {

	auto input_component = Input(&currentInput, "Type your message here");

	auto textContent = Renderer([=] {
		Elements message_elements;

		chatMutex.lock();
		for (const auto& msg : chatHistory) {

			char timeString[std::size("hh:mm:ssZ")];
			std::strftime(std::data(timeString), std::size(timeString),
				"%T", std::gmtime(&msg.receiveTime));
			std::string time_str = "[" + std::string(timeString) + "]";
			auto styled_time = ftxui::text(time_str) | ftxui::color(ftxui::Color::DeepSkyBlue1);
			std::string sender = msg.chatMessage.sender();
			auto styled_sender = ftxui::text("          " + sender) | ftxui::color(ftxui::Color::Yellow);
			std::string message = msg.chatMessage.message();
			auto styled_message = ftxui::text(message);


			message_elements.push_back(hbox(
			{
				styled_time,
				styled_sender,
				separator(),
				styled_message,
			}));
		}
		chatMutex.unlock();

		return vbox(message_elements) | focusPositionRelative(0, scroll_y) | frame | flex;
		});

	SliderOption<float> option_y;
	option_y.value = &scroll_y;
	option_y.min = 0.f;
	option_y.max = 1.f;
	option_y.increment = 0.01f;
	option_y.direction = Direction::Down;
	option_y.color_active = Color::Yellow;
	option_y.color_inactive = Color::YellowLight;
	auto scrollbar_y = Slider(option_y);

	auto main_component = Container::Vertical({
		Container::Horizontal({
			  textContent,
			  scrollbar_y,
		  }) | border | flex,
		input_component | border
		});


	auto main_renderer = Renderer(main_component, [&] {
		return vbox({
			main_component->Render()
			});
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

		int leftTries = timeoutTries;
		fd_set readSet;
		FD_ZERO(&readSet);
		FD_SET(clientSocket, &readSet);
		timeval timeout{ .tv_sec = pingTimeoutSeconds, .tv_usec = 0 };
		int ready = select(clientSocket + 1, &readSet, nullptr, nullptr, &timeout);
		char buffer[1024];
		if (ready > 0 && FD_ISSET(clientSocket, &readSet))
		{
			int receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
			if (receivedBytes <= 0) {
				file_logger->warn("recv bytes: {}", receivedBytes);
				if (LOGGING)
				{
					file_logger->error("Server disconnected");
				}
				EndProcess();
				break;
			}

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

				ChatMessage chatMessage = GetChatMessage(envelope.payload(), GetSendTypeString(envelope.sendtype()));
				AddChatMessage(chatMessage);
				break;
			}
			case MessageType::PING:
			{
				SendToServer(envelope);
				lastPingTime = std::chrono::steady_clock::now();
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
				case CommandType::CREATE_ROOM:
				{
					file_logger->info("Received {} or {} or {}", std::to_string(CommandType::HELP), std::to_string(CommandType::ROOM_LIST), std::to_string(CommandType::INVALID));

					ChatMessage chatMessage = GetChatMessage(cres.response(), GetSendTypeString(envelope.sendtype()));
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::NICKNAME:
				{
					file_logger->info("Received {}", std::to_string(CommandType::NICKNAME));

					clientUser.ParseFromString(cres.response());

					ChatMessage chatMessage = GetChatMessage("New nickname " + clientUser.name(), GetSendTypeString(envelope.sendtype()));
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::JOIN_ROOM:
				{
					file_logger->info("Received {}", std::to_string(CommandType::JOIN_ROOM));

					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Connected to the channel.", GetSendTypeString(envelope.sendtype()));
					AddChatMessage(chatMessage);

					break;
				}
				case CommandType::LEAVE_ROOM:
				{
					file_logger->info("Received {}", std::to_string(CommandType::LEAVE_ROOM));

					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Disconnected from the channel.", GetSendTypeString(envelope.sendtype()));
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

			auto now = std::chrono::steady_clock::now();
			if (now - lastPingTime > std::chrono::seconds(pingTimeoutSeconds))
			{
				if (LOGGING)
				{
					file_logger->error("Server disconnected");
				}
				//didn't received PING for timeout time.
				EndProcess();
				break;
			}
		}
		else if (ready == 0) // no data, timeout occured
		{
			if (leftTries == 0)
			{
				if (LOGGING)
				{
					file_logger->error("Server disconnected");
				}
				EndProcess();
				break;
			}
			file_logger->error("Select timeout has occured, retrying...");
			AddChatMessage(GetChatMessage("Server timeout, retrying...", GetSendTypeString(MessageSendType::LOCAL)));
			leftTries--;
			continue;
		}
	}
}

int main()
{
	sendTypeMappings = {
		{ MessageSendType::LOCAL, "[LOCAL]"},
		{ MessageSendType::GLOBAL, "[GLOBAL]"},
		{ MessageSendType::WITHIN_ROOM, "[ROOM]"},
		{ MessageSendType::WITHIN_ROOM_EXCEPT_THIS, "[ROOM]"}
	};
	file_logger = spdlog::rotating_logger_mt("file_logger", "logs/logs.log", 1024 * 1024, 3);
	file_logger->flush_on(spdlog::level::level_enum::info);
	spdlog::set_default_logger(file_logger);

	file_logger->info("-----------------------------------PROCESS START-----------------------------------");

	std::string json_config_file_path = "config.json";
	std::ifstream config_file(json_config_file_path);
	if (!config_file.is_open()) {
		file_logger->error("Error: Could not open config file: " + json_config_file_path);
		return 1;
	}

	std::string json_content((std::istreambuf_iterator<char>(config_file)),
		std::istreambuf_iterator<char>());
	config_file.close();

	SimpleServerConfig* serverConfig = new SimpleServerConfig();
	google::protobuf::util::JsonStringToMessage(json_content, serverConfig);

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		if (LOGGING)
		{
			file_logger->error("WSAStartup failed");
		}
		return 0;
	}

	uiThread = std::thread(UpdateChat);
	uiThread.detach();

	ChatMessage chatMessage;

	//create default user, its ID will be adjusted on /setnick command and room on /joinRoom or /leaveRoom
	clientUser = ClientUser{};
	clientUser.set_name("UserUnknown");
	clientUser.set_id(-1);
	clientUser.set_connectedroomid(-1);


	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		if (LOGGING)
		{
			file_logger->error("Socket creation failed: {}", WSAGetLastError());
		}
		WSACleanup();
		return 0;
	}


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, serverConfig->address().c_str(), &serverAddr.sin_addr);
	serverAddr.sin_port = htons(serverConfig->port());

	if (LOGGING)
	{
		file_logger->info("Trying to connect to server. address:{} port:{}", serverConfig->address(), serverConfig->port());
	}

	int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	if (returnCall == SOCKET_ERROR)
	{
		if (LOGGING)
		{
			file_logger->error("Connection error return call: {} socketVal: {}", returnCall, clientSocket);
		}
		EndProcess();
		return 0;
	}

	if (LOGGING) 
	{
		file_logger->info("Connection established! address:{} port: {}", serverConfig->address(), serverConfig->port());
	}
	chatMessage = GetChatMessage("/help to see commands", GetSendTypeString(MessageSendType::LOCAL));
	AddChatMessage(chatMessage);

	lastPingTime = std::chrono::steady_clock::now();


	syncChatThread = std::thread(ListenForMessages, clientSocket);
	syncChatThread.join();

	return 0;
}


