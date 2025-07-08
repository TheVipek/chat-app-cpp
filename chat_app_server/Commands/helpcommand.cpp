#include "helpcommand.h"

void HelpCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    SPDLOG_LOGGER_INFO(file_logger, "Help Command Process");
    std::string helperOutput;
    helperOutput += "AVAILABLE COMMANDS\n";
    helperOutput += "/setnick NEW_NICKNAME - set your nickname\n";
    helperOutput += "/joinRoom ROOM_NAME - join room with specific name\n";
    helperOutput += "/leaveRoom - leave your current room\n";
    helperOutput += "/roomList - prints out all publicly available servers.\n";
    helperOutput += "/createRoom NAME MAX_CONNECTIONS IS_PUBLIC PASSWORD(OPTIONAL!) - creates room.";
    CommandResponse cres{};
    cres.set_response(helperOutput);
    cres.set_type(CommandType::HELP);
    envelope.set_payload(cres.SerializeAsString());


    SPDLOG_LOGGER_INFO(file_logger, "Sending to client.");
    server->Send(envelope, senderSocket);
}