#pragma once

#include "clientchatmessage.h"

//protobuf
#include "communication.pb.h"


//spdlog
#include <spdlog/spdlog.h>

//Windows
#include <winsock2.h>
#include <ws2tcpip.h>

#include <memory>  
#include <mutex>   
#include <string>  
#include <vector> 


#define MAX_CHAT_HISTORY 100

namespace ChatApp
{
	class ChatLogic
	{
	public:
		ChatLogic(std::shared_ptr<spdlog::logger> _file_logger);
		~ChatLogic();
		void Initialize(SOCKET clientSocket);
		SOCKET clientSocket;

		int timeoutTries = 3;

		void ListenForMessages();
		bool SendToServer(Envelope envelope);
		ChatMessage GetChatMessage(std::string msg, std::string sender, ChatMessageType type);
		void AddChatMessage(ChatMessage message);

		std::vector<ClientChatMessage> GetChatHistory();
		ClientUserList GetAllUsers();
		ClientUser GetClient();
	private:
		bool initialized = false;
		ClientUser clientUser;
		std::mutex clientUserMtx;
		ClientUserList allUsers;
		std::mutex allUsersMtx;
		std::vector<ClientChatMessage> chatHistory;
		std::mutex chatHistoryMtx;
		std::shared_ptr<spdlog::logger> file_logger;
	};
}
