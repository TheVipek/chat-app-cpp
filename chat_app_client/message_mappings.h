#pragma once

//protobuf
#include "communication.pb.h"

//ftxui
#include "ftxui/screen/color.hpp"  

#include "map"
#include <string>

namespace ChatApp
{
	const std::map<ChatMessageType, ftxui::Color> colorDependingOnChatMessage{
		{ ChatMessageType::MESSAGE_IN_ROOM, ftxui::Color::Yellow},
		{ ChatMessageType::INFORMATION, ftxui::Color::RedLight},
		{ ChatMessageType::WHISPER, ftxui::Color::Magenta}
	};

	const ftxui::Color colorChatMessageDefault = ftxui::Color::IndianRedBis;
	const ftxui::Color timeChatMessageColor = ftxui::Color::DeepSkyBlue1;
	const ftxui::Color colorOfRoomText = ftxui::Color::Gold1;

	const std::map<MessageSendType, std::string> sendTypeMappings = {
		{ MessageSendType::LOCAL, "[LOCAL]"},
		{ MessageSendType::GLOBAL, "[GLOBAL]"},
		{ MessageSendType::WITHIN_ROOM, "[ROOM]"},
		{ MessageSendType::WITHIN_ROOM_EXCEPT_THIS, "[ROOM]"}
	};
	const std::string messageSendTypeDefault = "Unknown MessageSendType";

	const std::map<MessageType, std::string> messageTypeMappings = {
		{ MessageType::CHAT_MESSAGE, "CHAT_MESSAGE"},
		{ MessageType::COMMAND, "COMMAND"},
		{ MessageType::PING, "PING"},
		{ MessageType::USER_JOIN_ROOM, "USER_JOIN_ROOM"},
		{ MessageType::USER_LEAVE_ROOM, "USER_LEAVE_ROOM" }

	};
	const std::string messageTypeDefault = "Unknown MessageType";

	const std::map<CommandType, std::string> commandTypeMappings = {
		{ CommandType::CREATE_ROOM, "CREATE_ROOM"},
		{ CommandType::HELP, "HELP"},
		{ CommandType::INVALID, "INVALID"},
		{ CommandType::JOIN_ROOM, "JOIN_ROOM"},
		{ CommandType::LEAVE_ROOM, "LEAVE_ROOM"},
		{ CommandType::NICKNAME, "NICKNAME"},
		{ CommandType::ROOM_LIST, "ROOM_LIST"},
		{ CommandType::USER_WHISPER, "WHISPER"}
	};
	const std::string commandTypeDefault = "Unknown CommandType";
}