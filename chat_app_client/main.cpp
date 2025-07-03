#pragma once
#include "communication.pb.h"
#include <ftxui/dom/elements.hpp>  // for Elements, gridbox, Fit, operator|, text, border, Element
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/node.hpp"      // for Render
#include "ftxui/screen/color.hpp"  // for ftxui
#include "ftxui/component/component.hpp"

#include <stdio.h>  // for getchar
#include <memory>                   // for allocator, shared_ptr
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
#pragma comment(lib, "ws2_32.lib")

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
	std::string serializedEnvelope = envelope.SerializeAsString();
	int sendResult = send(clientSocket, serializedEnvelope.data(), serializedEnvelope.size(), 0);

	if (sendResult == SOCKET_ERROR) {
		auto chatMessage = GetChatMessage("send failed: " + std::to_string(WSAGetLastError()), "[ERROR]");
		AddChatMessage(chatMessage);
		closesocket(clientSocket);

		return false;
	}

	return true;
}

void HandleInput() {
	if (currentInput.empty())
		return;

	if (currentInput[0] == '/') // command
	{
		auto msg = GetChatMessage(currentInput, "[VISIBLE ONLY BY YOU]");
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
		auto msg = GetChatMessage("You need to set nickname and join any room to send messages.", "[LOCAL]");
		AddChatMessage(msg);
	}


	currentInput.clear();
}



void UpdateChat() {
	auto message_component = Container::Vertical({});

	Component input_component = Input(&currentInput, "Type your message here");

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
				auto chatMessage = GetChatMessage("Server disconnected", "[ERROR]");
				AddChatMessage(chatMessage);
				break;
			}
		}
		else {

			Envelope envelope;
			envelope.ParseFromString(buffer);

			switch (envelope.type())
			{
			case MessageType::CHAT_MESSAGE: {
				ChatMessage chatMessage;
				chatMessage.ParseFromString(envelope.payload());
				AddChatMessage(chatMessage);
				break;
			}
			case MessageType::COMMAND: {
				CommandResponse cres;
				cres.ParseFromString(envelope.payload());

				//in very simple way for now, just want to have it working correctly
				if (cres.type() == CommandType::HELP)
				{
					ChatMessage chatMessage = GetChatMessage(cres.response(), "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				else if (cres.type() == CommandType::NICKNAME)
				{
					clientUser.ParseFromString(cres.response());

					ChatMessage chatMessage = GetChatMessage("Set new nickname to: " + clientUser.name(), "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				else if (cres.type() == CommandType::JOIN_ROOM)
				{
					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Connected to the channel.", "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				else if (cres.type() == CommandType::LEAVE_ROOM)
				{
					clientUser.ParseFromString(cres.response());
					ChatMessage chatMessage = GetChatMessage("Disconnected from the channel.", "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				else if (cres.type() == CommandType::ROOM_LIST)
				{
					ChatMessage chatMessage = GetChatMessage(cres.response(), "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				else if (cres.type() == CommandType::INVALID)
				{
					ChatMessage chatMessage = GetChatMessage(cres.response(), "[VISIBLE ONLY BY YOU]");
					AddChatMessage(chatMessage);
				}
				break;
			}
			case MessageType::USER_JOIN_ROOM:{
				
				ChatMessage chatMessage = GetChatMessage(envelope.payload(), "[EVERYONE]");
				AddChatMessage(chatMessage);
				break;
			}
			case MessageType::USER_LEAVE_ROOM: {
				ChatMessage chatMessage = GetChatMessage(envelope.payload(), "[EVERYONE]");
				AddChatMessage(chatMessage);
				break;
			}
			default:
				break;
			}
		}
	}

	closesocket(clientSocket);
}

int main()
{
	uiThread = std::thread(UpdateChat);
	uiThread.detach();

	WSADATA wsaData;

	ChatMessage chatMessage;
	clientUser = ClientUser{};
	clientUser.set_name("UserUnknown");
	clientUser.set_id(-1);
	clientUser.set_connectedroomid(-1);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		chatMessage = GetChatMessage("WSAStartup failed", "[ERROR]");
		AddChatMessage(chatMessage);
		return 1;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		chatMessage = GetChatMessage("Socket creation failed: " + WSAGetLastError(), "[ERROR]");
		AddChatMessage(chatMessage);
		WSACleanup();
		return 1;
	}


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(SERVER_PORT);

	chatMessage = GetChatMessage("Trying to connect to server...", "[INFO]");
	AddChatMessage(chatMessage);

	int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	if (returnCall == SOCKET_ERROR)
	{
		chatMessage = GetChatMessage("Connection error", "[ERROR]");
		AddChatMessage(chatMessage);
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	chatMessage = GetChatMessage("Connection established!", "[INFO]");
	AddChatMessage(chatMessage);
	chatMessage = GetChatMessage("Welcome on the server! (/help to see commands)", "[INFO]");
	AddChatMessage(chatMessage);



	syncChatThread = std::thread(ListenForMessages, clientSocket);
	syncChatThread.join();

	WSACleanup();
	return 1;
}


