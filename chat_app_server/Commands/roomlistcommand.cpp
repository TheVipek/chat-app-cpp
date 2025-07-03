#include "roomlistcommand.h"

void RoomListCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    MessageSendType sendType;

    printf("room list process\n");
    sendType = MessageSendType::LOCAL;
    CommandResponse cres{};

    std::string response;
    response += "\n* - NEEDED \n";

    auto roomContainers = server->GetRoomContainers();
    for (auto roomContainer : roomContainers) {
        auto room = roomContainer->room;
        auto connections = roomContainer->usersInRoom;
        std::string needPassword = room->haspassword() ? "*" : "";
        response += room->name() + " Password(" + needPassword + ") Connections:" + std::to_string(connections.size()) + "/" + std::to_string(room->maxconnections()) + "\n";
    }

    cres.set_response(response);
    cres.set_type(CommandType::ROOM_LIST);
    envelope.set_payload(cres.SerializeAsString());

    printf("sending command response to client\n");
    server->Send(envelope, sendType, senderSocket);
}