#include "chatlogic.h"
#include "helpers.h"
#include "message_mappings.h"
#include <google/protobuf/util/json_util.h>

using namespace ChatApp;


ChatLogic::ChatLogic(std::shared_ptr<spdlog::logger> _file_logger)
{
	file_logger = _file_logger;
	//create default user, its ID will be adjusted on /setnick command and room on /joinRoom or /leaveRoom
	clientUser = ClientUser{};
	clientUser.set_name("UserUnknown");
	clientUser.set_id(-1);
	clientUser.set_connectedroomid(-1);
}
ChatLogic::~ChatLogic()
{
	if (initialized)
	{
		closesocket(clientSocket);
	}
}
void ChatLogic::Initialize(SOCKET _clientSocket)
{
	clientSocket = _clientSocket;
	initialized = true;
}
bool ChatLogic::SendToServer(Envelope envelope) {
	SPDLOG_LOGGER_INFO(file_logger, "Send To Server");
	std::string serializedEnvelope = envelope.SerializeAsString();

	uint32_t envelopeSize = static_cast<uint32_t>(serializedEnvelope.size());
	uint32_t networkOrderSize = htonl(envelopeSize);

	// Send length
	const char* frameData = reinterpret_cast<const char*>(&networkOrderSize);
	int frameTotalSent = 0;
	while (frameTotalSent < 4) {
		int n = send(clientSocket, frameData + frameTotalSent, 4 - frameTotalSent, 0);
		if (n == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) return false;
			SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", clientSocket, err);
			return true;
		}
		frameTotalSent += n;
	}

	// Send payload
	const char* envelopeBuffer = serializedEnvelope.data();
	int totalSent = 0;
	while (totalSent < envelopeSize) {
		int n = send(clientSocket, envelopeBuffer + totalSent, envelopeSize - totalSent, 0);
		if (n == SOCKET_ERROR) {
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) return false;
			SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", clientSocket, err);
			return true;
		}
		totalSent += n;
	}

	SPDLOG_LOGGER_INFO(file_logger, "Send Finish");
	return true;
}

void ChatLogic::ListenForMessages()
{
	if (!initialized)
	{
		SPDLOG_LOGGER_ERROR(file_logger, "Client is not initialized");
		return;
	}
	int leftTries = timeoutTries;
	while (true) {


		int receivedBytes;
		char frameSizeBuffer[sizeof(uint32_t)];
		receivedBytes = recv(clientSocket, frameSizeBuffer, sizeof(uint32_t), 0);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == SOCKET_ERROR) {
				SPDLOG_LOGGER_ERROR(file_logger, "Server disconnected, recv bytes {}", receivedBytes);
				throw SOCKET_ERROR;
			}

			SPDLOG_LOGGER_ERROR(file_logger, "Unexpected error {}", receivedBytes);
			continue;
		}
		int frameSize = *reinterpret_cast<uint32_t*>(frameSizeBuffer);
		frameSize = ntohl(frameSize);


		int allReceivedBytes = 0;

		std::vector<char> buffer(frameSize);
		receivedBytes = recv(clientSocket, buffer.data(), frameSize, 0);
		if (receivedBytes <= 0)
		{
			if (receivedBytes == SOCKET_ERROR) {
				SPDLOG_LOGGER_ERROR(file_logger, "Server disconnected, recv bytes {}", receivedBytes);
				throw SOCKET_ERROR;
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


		auto sendType = envelope.sendtype();
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
			ChatMessage chatMessage = GetChatMessage(envelope.payload(), GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
			AddChatMessage(chatMessage);
			break;
		}
		case MessageType::USER_LEAVE_ROOM:
		{
			ChatMessage chatMessage = GetChatMessage(envelope.payload(), GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
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
			SendToServer(envelope);
			break;
		}
		case MessageType::ALL_USERS_UPDATE:
		{
			SPDLOG_LOGGER_INFO(file_logger, "All users update!");
			allUsersMtx.lock();
			allUsers.ParseFromString(envelope.payload());
			allUsersMtx.unlock();

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
				ChatMessage chatMessage = GetChatMessage(cres.response(), GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::NICKNAME:
			{
				clientUser.ParseFromString(cres.response());

				ChatMessage chatMessage = GetChatMessage("New nickname " + clientUser.name(), GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::JOIN_ROOM:
			{
				clientUser.ParseFromString(cres.response());
				ChatMessage chatMessage = GetChatMessage("Connected to the channel.", GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
				AddChatMessage(chatMessage);

				break;
			}
			case CommandType::LEAVE_ROOM:
			{
				clientUser.ParseFromString(cres.response());
				ChatMessage chatMessage = GetChatMessage("Disconnected from the channel.", GetEnumValue(sendType, sendTypeMappings, messageSendTypeDefault), ChatMessageType::INFORMATION);
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

ChatMessage ChatLogic::GetChatMessage(std::string msg, std::string sender, ChatMessageType type)
{
	ChatMessage chatMessage;
	chatMessage.set_message(msg);
	chatMessage.set_sender(sender);
	chatMessage.set_messagetype(type);
	return chatMessage;
}

void ChatLogic::AddChatMessage(ChatMessage message)
{
	std::lock_guard<std::mutex> lock(chatHistoryMtx);
	ClientChatMessage clientChatMessage;
	time_t timestamp;
	time(&timestamp);

	clientChatMessage.receiveTime = timestamp;
	clientChatMessage.chatMessage = message;
	chatHistory.push_back(clientChatMessage);
}

std::vector<ClientChatMessage> ChatLogic::GetChatHistory()
{
	std::lock_guard<std::mutex> lock(chatHistoryMtx);
	return chatHistory;
}

ClientUserList ChatLogic::GetAllUsers()
{
	std::lock_guard<std::mutex> lock(allUsersMtx);
	ClientUserList copy;
	copy.CopyFrom(allUsers);
	return copy;
}

ClientUser ChatLogic::GetClient()
{
	std::lock_guard<std::mutex> lock(clientUserMtx);
	ClientUser copy;
	copy.CopyFrom(clientUser);
	return copy;
}