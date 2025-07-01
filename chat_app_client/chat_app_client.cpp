#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#pragma comment(lib, "ws2_32.lib")

#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT 8080

struct ChatMessage {
	std::string message;
	bool displayed;
};
std::string userName;
std::string currentMessage;
std::vector<ChatMessage> chatHistory;  // Stores chat messages
std::mutex chatMutex;
std::thread inputThread;
std::thread uiThread;
std::thread syncChatThread;



void ClearConsole() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);

	// Clear the screen
	COORD topLeft = { 0, 0 };
	DWORD written;
	FillConsoleOutputCharacter(hConsole, ' ',
		consoleInfo.dwSize.X * consoleInfo.dwSize.Y,
		topLeft, &written);
	SetConsoleCursorPosition(hConsole, topLeft);
}
void PrintChatUI() {
	chatMutex.lock();

	bool anyDisplayed = false;
	for (auto& msg : chatHistory) {
		if (!msg.displayed)
		{
			std::cout << "\r" << msg.message << "\n";
			msg.displayed = true;

			anyDisplayed = true;
		}
	}

	if (anyDisplayed)
	{
		std::cout << "> ";
	}

	chatMutex.unlock();
}

void UpdateChat() {
	while (true) {
		PrintChatUI();

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void AddChatMessage(const std::string& msg)
{
	ChatMessage chatMessage{};
	chatMessage.message = msg;
	chatMessage.displayed = false;
	chatMutex.lock();
	chatHistory.push_back(chatMessage);
	chatMutex.unlock();
}

void HandleInput(SOCKET clientSocket) {
	while (true) {
		std::cout << "> ";
		std::getline(std::cin, currentMessage);
		currentMessage += "\0";
		if (currentMessage.empty()) continue;

		int sendResult = send(clientSocket, currentMessage.c_str(), currentMessage.length(), 0);

		if (sendResult == SOCKET_ERROR) {
			AddChatMessage("[Error] send failed: " + std::to_string(WSAGetLastError()));
			break;
		}

	}

	closesocket(clientSocket);
}

void ListenForMessages(SOCKET clientSocket)
{
	while (true) {
		char buffer[256];
		int receivedBytes = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (receivedBytes <= 0) {
			if (receivedBytes == 0) {
				AddChatMessage("[Server disconnected]");
				break;
			}
		}
		else {
			AddChatMessage("USER: " + std::string(buffer, receivedBytes));
		}
	}

	closesocket(clientSocket);
}

int main()
{
	//int id = rand() % 10000000;
	//userName = "USER " + id;
	//WSADATA wsaData;
	//if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
	//	std::cerr << "WSAStartup failed.\n";
	//	return 1;
	//}

	//SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//if (clientSocket == INVALID_SOCKET)
	//{
	//	std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
	//	WSACleanup();
	//	return 1;
	//}


	//sockaddr_in serverAddr;
	//serverAddr.sin_family = AF_INET;
	//inet_pton(AF_INET, SERVER_ADDRESS, &serverAddr.sin_addr);
	//serverAddr.sin_port = htons(SERVER_PORT);

	//std::cerr << "Trying to connect to server..." << "\n";
	//int returnCall = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

	//if (returnCall == SOCKET_ERROR)
	//{
	//	std::cerr << "Connection error" << "\n";
	//	closesocket(clientSocket);
	//	WSACleanup();
	//	return 1;
	//}


	//ClearConsole();

	//std::cerr << "Connection established!" << "\n";


	//uiThread = std::thread(UpdateChat);
	//inputThread = std::thread(HandleInput, clientSocket);
	//syncChatThread = std::thread(ListenForMessages, clientSocket);

	//uiThread.join();
	//inputThread.join();
	//syncChatThread.join();

	//WSACleanup();
	//return 1;

	using namespace ftxui;

	// Create a simple document with three text elements.
	Element document = hbox({
	  text("left") | border,
	  text("middle") | border | flex,
	  text("right") | border,
		});

	// Create a screen with full width and height fitting the document.
	auto screen = Screen::Create(
		Dimension::Full(),       // Width
		Dimension::Fit(document) // Height
	);

	// Render the document onto the screen.
	Render(screen, document);

	// Print the screen to the console.
	screen.Print();
}


