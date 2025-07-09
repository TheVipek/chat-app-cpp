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
int timeoutTries = 3;
std::mutex chatMutex;
std::thread uiThread;
std::thread pingThread;
std::thread syncChatThread;
std::shared_ptr<spdlog::logger> file_logger;
uint32_t frameSize = 0;
float scroll_y = 1.f;
std::map<MessageSendType, std::string> sendTypeMappings = {
		{ MessageSendType::LOCAL, "[LOCAL]"},
		{ MessageSendType::GLOBAL, "[GLOBAL]"},
		{ MessageSendType::WITHIN_ROOM, "[ROOM]"},
		{ MessageSendType::WITHIN_ROOM_EXCEPT_THIS, "[ROOM]"}
};
std::string messageSendTypeDefault = "Unknown MessageSendType";

std::map<MessageType, std::string> messageTypeMappings = {
	{ MessageType::CHAT_MESSAGE, "CHAT_MESSAGE"},
	{ MessageType::COMMAND, "COMMAND"},
	{ MessageType::PING, "PING"},
	{ MessageType::USER_JOIN_ROOM, "USER_JOIN_ROOM"},
	{ MessageType::USER_LEAVE_ROOM, "USER_LEAVE_ROOM" }

};
std::string messageTypeDefault = "Unknown MessageType";

std::map<CommandType, std::string> commandTypeMappings = {
	{ CommandType::CREATE_ROOM, "CREATE_ROOM"},
	{ CommandType::HELP, "HELP"},
	{ CommandType::INVALID, "INVALID"},
	{ CommandType::JOIN_ROOM, "JOIN_ROOM"},
	{ CommandType::LEAVE_ROOM, "LEAVE_ROOM"},
	{ CommandType::NICKNAME, "NICKNAME"},
	{ CommandType::ROOM_LIST, "ROOM_LIST"},
	{ CommandType::USER_WHISPER, "WHISPER"}
};	
std::string commandTypeDefault = "Unknown CommandType";


std::map<ChatMessageType, ftxui::Color> colorDependingOnChatMessage{
		{ ChatMessageType::MESSAGE_IN_ROOM, ftxui::Color::Yellow},
		{ ChatMessageType::INFORMATION, ftxui::Color::RedLight},
		{ ChatMessageType::WHISPER, ftxui::Color::Magenta}
};
ftxui::Color colorChatMessageDefault = ftxui::Color::IndianRedBis;
ftxui::Color timeChatMessageColor = ftxui::Color::DeepSkyBlue1;
ftxui::Color colorOfRoomText = ftxui::Color::Gold1;
template<typename T, typename U>
U GetEnumValue(T enumValue, std::map<T, U>& map, U& defaultValue)
{
	if (map.contains(enumValue))
	{
		return map[enumValue];
	}

	return defaultValue;
}

void EndProcess()
{
	closesocket(clientSocket);
	WSACleanup();
	SPDLOG_LOGGER_INFO(file_logger, "-----------------------------------PROCESS END-----------------------------------");
	exit(1);
}

ChatMessage GetChatMessage( std::string msg,  std::string sender, ChatMessageType type)
{
	ChatMessage chatMessage;
	chatMessage.set_message(msg);
	chatMessage.set_sender(sender);
	chatMessage.set_messagetype(type);
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

bool SendToServer(Envelope envelope, SOCKET socket) {
	SPDLOG_LOGGER_INFO(file_logger, "Send To Server");
	std::string serializedEnvelope = envelope.SerializeAsString();

	uint32_t envelopeSize = static_cast<uint32_t>(serializedEnvelope.size());
	uint32_t networkOrderSize = htonl(envelopeSize);

	// Send length
	const char* frameData = reinterpret_cast<const char*>(&networkOrderSize);
	int frameTotalSent = 0;
	while (frameTotalSent < 4) {
		int n = send(socket, frameData + frameTotalSent, 4 - frameTotalSent, 0);
		if (n == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) return false;
			SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", socket, err);
			return true;
		}
		frameTotalSent += n;
	}

	// Send payload
	const char* envelopeBuffer = serializedEnvelope.data();
	int totalSent = 0;
	while (totalSent < envelopeSize) {
		int n = send(socket, envelopeBuffer + totalSent, envelopeSize - totalSent, 0);
		if (n == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) return false;
			SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", socket, err);
			return true;
		}
		totalSent += n;
	}

	SPDLOG_LOGGER_INFO(file_logger, "Send Finish");
	return true;
}

void HandleInput() {
	if (currentInput.empty())
		return;

	if (currentInput[0] == '/') // command
	{

		SPDLOG_LOGGER_INFO(file_logger, "Sending command: {}", currentInput);

		// commented it, because it was making chat less readable
		// 
		//auto msg = GetChatMessage(currentInput, GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
	    //AddChatMessage(msg);
		
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
		SendToServer(envelope, clientSocket);
	}
	else if(clientUser.id() != -1 && clientUser.connectedroomid() != -1)
	{

		SPDLOG_LOGGER_INFO(file_logger, "Send chat message: {}", currentInput);
		std::string name = std::format("{}#{}", clientUser.name(), clientUser.id());
		auto msg = GetChatMessage(currentInput, name, ChatMessageType::MESSAGE_IN_ROOM);

		Envelope envelope{};
		envelope.set_type(MessageType::CHAT_MESSAGE);
		envelope.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
		envelope.set_payload(msg.SerializeAsString());

		if (SendToServer(envelope, clientSocket))
		{
			AddChatMessage(msg);
		}
	}
	else 
	{

		SPDLOG_LOGGER_WARN(file_logger, "Cannot send chat message: {}", currentInput);
		auto msg = GetChatMessage("You need to set nickname and join any room to send messages.", GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
		AddChatMessage(msg);
	}


	currentInput.clear();
}

void UpdateChat() {

	auto input_component = Input(&currentInput, "Type your message here");

	auto inputContent = Renderer([=] {
		return vbox(
			{
				input_component->Render()
			}) | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100) | borderLight;
		});
	
	auto headerContent = Renderer([=] {
		Element channelName = text(clientUser.roomname() == "" ? "Default Room" : clientUser.roomname() + " Room") | ftxui::color(colorOfRoomText);
		return vbox(
			{
				channelName | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100),
				separator()
		});
	});

	auto textContent = Renderer([=] {
		Elements message_elements;

		chatMutex.lock();
		for (const auto& msg : chatHistory) {

			char timeString[std::size("hh:mm:ssZ")];
			std::strftime(std::data(timeString), std::size(timeString),
				"%T", std::gmtime(&msg.receiveTime));
			std::string time_str = "[" + std::string(timeString) + "]";
			auto styled_time = ftxui::text(time_str) | ftxui::color(timeChatMessageColor);
			std::string sender = msg.chatMessage.sender();
			auto styled_sender = ftxui::text(sender) | ftxui::color(GetEnumValue(msg.chatMessage.messagetype(), colorDependingOnChatMessage, colorChatMessageDefault));
			std::string message = msg.chatMessage.message();
			auto styled_message = ftxui::paragraph(message);


			message_elements.push_back(hbox(
			{
				styled_time,
				styled_sender | align_right | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 30) ,
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


	auto allUsersContent = Renderer([=] {

		return vbox();
		});


	auto main_component = Container::Vertical({
		headerContent ,
		Container::Horizontal({
			Container::Horizontal({
				textContent,
			    scrollbar_y,
			}) | borderLight | flex,
			Container::Horizontal({
				allUsersContent	
			}) | borderLight | flex | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 20)
		  }) | flex,
		input_component | size(WidthOrHeight::WIDTH, Constraint::EQUAL, 100) | borderLight,
		}) | borderLight | size(WidthOrHeight::HEIGHT, Constraint::EQUAL, 100);


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

	int leftTries = timeoutTries;
	while (true) {

		
		int receivedBytes;
		char frameSizeBuffer[sizeof(uint32_t)];
		receivedBytes = recv(clientSocket, frameSizeBuffer, sizeof(uint32_t), 0);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == SOCKET_ERROR) {
				SPDLOG_LOGGER_ERROR(file_logger, "Server disconnected, recv bytes {}", receivedBytes);
				EndProcess();
				break;
			}

			SPDLOG_LOGGER_ERROR(file_logger, "Unexpected error {}", receivedBytes);
			continue;
		}
		frameSize = *reinterpret_cast<uint32_t*>(frameSizeBuffer);
		frameSize = ntohl(frameSize);


		int allReceivedBytes = 0;

		std::vector<char> buffer(frameSize);
		receivedBytes = recv(clientSocket, buffer.data(), frameSize, 0);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == SOCKET_ERROR) {
				SPDLOG_LOGGER_ERROR(file_logger, "Server disconnected, recv bytes {}", receivedBytes);
				EndProcess();
				break;
			}

			SPDLOG_LOGGER_ERROR(file_logger, "Unexpected error {}", receivedBytes);
			continue;
		}


		
		leftTries = timeoutTries;
		SPDLOG_LOGGER_INFO(file_logger, "Message from server received");

		Envelope envelope{};
		envelope.ParseFromString(buffer.data());

		SPDLOG_LOGGER_INFO(file_logger, "Received {}", GetEnumValue(envelope.type(), messageTypeMappings, messageTypeDefault));


		switch (envelope.type())
		{
		case MessageType::CHAT_MESSAGE:
		{
			ChatMessage chatMessage;
			chatMessage.ParseFromString(envelope.payload());
			AddChatMessage(chatMessage);
			break;
		}
		case MessageType::USER_JOIN_ROOM:
		{
			ChatMessage chatMessage = GetChatMessage(envelope.payload(), GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
			AddChatMessage(chatMessage);
			break;
		}
		case MessageType::USER_LEAVE_ROOM:
		{
			ChatMessage chatMessage = GetChatMessage(envelope.payload(), GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
			AddChatMessage(chatMessage);
			break;
		}
		case MessageType::PING:
		{
			SPDLOG_LOGGER_INFO(file_logger, "Send PING response to the server");
			envelope.Clear();
			envelope.set_type(MessageType::PING);
			envelope.set_sendtype(MessageSendType::LOCAL);
			envelope.set_payload("");
			SendToServer(envelope, clientSocket);
			break;
		}
		case MessageType::COMMAND:
		{
			CommandResponse cres;
			cres.ParseFromString(envelope.payload());

			SPDLOG_LOGGER_INFO(file_logger, "Received {}", GetEnumValue(cres.type(), commandTypeMappings, commandTypeDefault));

			switch (cres.type())
			{
			case CommandType::HELP:
			case CommandType::ROOM_LIST:
			case CommandType::INVALID:
			case CommandType::CREATE_ROOM:
			{
				ChatMessage chatMessage = GetChatMessage(cres.response(), GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::NICKNAME:
			{
				clientUser.ParseFromString(cres.response());

				ChatMessage chatMessage = GetChatMessage("New nickname " + clientUser.name(), GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::JOIN_ROOM:
			{
				clientUser.ParseFromString(cres.response());
				ChatMessage chatMessage = GetChatMessage("Connected to the channel.", GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::LEAVE_ROOM:
			{
				clientUser.ParseFromString(cres.response());
				ChatMessage chatMessage = GetChatMessage("Disconnected from the channel.", GetEnumValue(envelope.sendtype(), sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::USER_WHISPER:
			{
				WhisperData wd; 
				wd.ParseFromString(cres.response());

				ChatMessage chatMessage = GetChatMessage(wd.message(), wd.from(), ChatMessageType::WHISPER);
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

int main()
{

	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;    
	ss << std::put_time(std::localtime(&in_time_t), "%Y_%m_%d_%H_%M_%S");

	file_logger = spdlog::rotating_logger_mt("file_logger", "logs/logs_" + ss.str() + "_" + std::to_string(getpid()) + +".log", 1024 * 1024, 3);
	file_logger->flush_on(spdlog::level::level_enum::info);
	spdlog::set_default_logger(file_logger);
	//file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
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

	chatMessage = GetChatMessage("/help to see commands", GetEnumValue(MessageSendType::LOCAL, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
	AddChatMessage(chatMessage);


	syncChatThread = std::thread(ListenForMessages, clientSocket);
	syncChatThread.join();

	EndProcess();
}


