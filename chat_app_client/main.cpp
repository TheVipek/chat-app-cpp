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

SOCKET clientSocket;
std::string currentInput;

std::string userName;
std::vector<ChatMessage> chatHistory;
std::mutex chatMutex;
std::thread uiThread;
std::thread syncChatThread;

using namespace ftxui;

ChatMessage GetChatMessage( std::string msg,  std::string userName)
{
	ChatMessage chatMessage;
	chatMessage.set_message(msg);
	chatMessage.set_username(userName);

	return chatMessage;
}
void AddChatMessage( std::string msg,  std::string userName)
{
	ChatMessage chatMessage{};
	chatMessage.set_message(msg);
	chatMessage.set_username(userName);
	chatMutex.lock();
	chatHistory.push_back(chatMessage);
	chatMutex.unlock();
}
void AddChatMessage(ChatMessage message)
{
	chatMutex.lock();
	chatHistory.push_back(message);
	chatMutex.unlock();
}

void HandleInput() {
	if (currentInput.empty())
		return;

	auto msg = GetChatMessage(currentInput, userName);
	
	Envelope envelope;
	envelope.set_type(MessageType::CHAT_MESSAGE);
	envelope.set_payload(msg.SerializeAsString());

	std::string serializedEnvelope = envelope.SerializeAsString();

	int sendResult = send(clientSocket, serializedEnvelope.data(), serializedEnvelope.size(), 0);

	if (sendResult == SOCKET_ERROR) {
		AddChatMessage("send failed: " + std::to_string(WSAGetLastError()), "[ERROR]");
		closesocket(clientSocket);
	}
	else {
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
			message_elements.push_back(text(msg.username() + ":" + msg.message()));
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
	screen.Loop(component);
}




void ListenForMessages(SOCKET clientSocket)
{
	while (true) {
		char buffer[256];
		int receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (receivedBytes <= 0) {
			if (receivedBytes == 0) {
				AddChatMessage("Server disconnected", "[ERROR]");
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
			default:
				break;
			}
		}
	}

	closesocket(clientSocket);
}

int main()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distrib(0, 9999999);
	int id = distrib(gen);

	userName = "USER " + std::to_string(id);

	uiThread = std::thread(UpdateChat);
	uiThread.detach();

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		AddChatMessage("WSAStartup failed", "[ERROR]");
		return 1;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
	{
		AddChatMessage("Socket creation failed: " + WSAGetLastError(), "[ERROR]");
		WSACleanup();
		return 1;
	}


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(SERVER_PORT);

	AddChatMessage("Trying to connect to server...", "[INFO]");

	int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	if (returnCall == SOCKET_ERROR)
	{
		AddChatMessage("Connection error", "[ERROR]");
		closesocket(clientSocket);
		WSACleanup();
		return 1;
	}

	AddChatMessage("Connection established!", "[INFO]");
	AddChatMessage("Welcome to the chat!", "[INFO]");


	syncChatThread = std::thread(ListenForMessages, clientSocket);
	syncChatThread.join();

	WSACleanup();
	return 1;
}


