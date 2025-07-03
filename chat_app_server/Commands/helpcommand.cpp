#include "helpcommand.h"

void HelpCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);

    printf("help process\n");
    CommandResponse cres{};
    cres.set_response("AVAILABLE COMMANDS\n/setnick NEW_NICKNAME - set your nickname\n/joinRoom ROOM_NAME - join room with specific name\n/leaveRoom - leave your current room\n/roomList - prints out all publicly available servers.");
    cres.set_type(CommandType::HELP);
    envelope.set_payload(cres.SerializeAsString());

    printf("sending command response to client\n");
    server->Send(envelope, MessageSendType::LOCAL, senderSocket);
}