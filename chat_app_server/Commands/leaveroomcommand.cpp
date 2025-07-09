#include "leaveroomcommand.h"

void LeaveRoomCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    SPDLOG_LOGGER_INFO(file_logger, "Leave Room Command Process");

    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    auto user = server->GetUser(senderSocket);
    auto roomContainer = server->GetRoomContainer(user->connectedroomid());
    if (user->id() != -1 && user->connectedroomid() != -1 && roomContainer != nullptr)
    {
        ClientUser newUser{};
        newUser.CopyFrom(*user);
        newUser.set_connectedroomid(-1);

        CommandResponse cres{};
        std::string newUserData;
        newUser.SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::LEAVE_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        std::string envelopeParsed;
        envelope.SerializeToString(&envelopeParsed);

        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);



        envelope.set_type(MessageType::USER_LEAVE_ROOM);
        envelope.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
        std::string msg = user->name() + "has left the channel.";
        envelope.set_payload(msg);
        server->Send(envelope, senderSocket);

        //update client
        server->UpdateExistingUserData(senderSocket, -1, "", -1);
        SPDLOG_LOGGER_INFO(file_logger, "Removed user {} from room {}", user->name(), roomContainer->room->name());


        if (roomContainer->usersInRoom.size() == 0 && roomContainer->destroyOnEmpty)
        {
            server->RemoveRoom(roomContainer->room->name());


            Envelope envelope{};
            envelope.set_type(MessageType::USER_LEAVE_ROOM);
            envelope.set_sendtype(MessageSendType::LOCAL);
            std::string msg = "The room you have left has been destroyed, due to 0 users connected to it.";
            envelope.set_payload(msg);
            server->Send(envelope, senderSocket);
        }

        return;
    }

    CommandResponse cres{};
    SPDLOG_LOGGER_WARN(file_logger, "Cannot use command");
    cres.set_response("Cannot use this command now.");
    cres.set_type(CommandType::INVALID);
    envelope.set_payload(cres.SerializeAsString());

    SPDLOG_LOGGER_INFO(file_logger, "Send to client");
    server->Send(envelope, senderSocket);
}