#include "leaveroomcommand.h"

void LeaveRoomCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    file_logger->info("Leave Room Command Process");

    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    MessageSendType sendType;
    auto user = server->GetUser(senderSocket);
    auto roomContainer = server->GetRoomContainer(user->connectedroomid());
    if (user->id() != -1 && user->connectedroomid() != -1 && roomContainer != nullptr)
    {
        ClientUser newUser{};
        newUser.CopyFrom(*user);
        newUser.set_connectedroomid(-1);

        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        std::string newUserData;
        newUser.SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::LEAVE_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        std::string envelopeParsed;
        envelope.SerializeToString(&envelopeParsed);

        file_logger->info("Send to client");
        server->Send(envelope, sendType, senderSocket);


        Envelope envelope2{};
        sendType = MessageSendType::WITHIN_ROOM_EXCEPT_THIS;
        envelope2.set_type(MessageType::USER_LEAVE_ROOM);
        std::string msg = user->name() + "has left the channel.";
        envelope2.set_payload(msg);
        server->Send(envelope2, sendType, senderSocket);


        //remove old user
        roomContainer->RemoveUser(user);
        file_logger->info("Removed user {} from room {}", user->name(), roomContainer->room->name());
        return;
    }
    
    sendType = MessageSendType::LOCAL;
    CommandResponse cres{};
    file_logger->warn("Cannot use command");
    cres.set_response("Cannot use this command now.");
    cres.set_type(CommandType::INVALID);
    envelope.set_payload(cres.SerializeAsString());

    file_logger->info("Send to client");
    server->Send(envelope, sendType, senderSocket);
}