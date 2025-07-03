#include "leaveroomcommand.h"

void LeaveRoomCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
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

        printf("sending command response to client\n");
        server->Send(envelope, sendType, senderSocket);


        Envelope envelope2{};
        sendType = MessageSendType::WITHIN_ROOM_EXCEPT_THIS;
        envelope2.set_type(MessageType::USER_LEAVE_ROOM);
        std::string msg = user->name() + "has left the channel.";
        envelope2.set_payload(msg);
        server->Send(envelope2, sendType, senderSocket);


        //remove old user
        roomContainer->RemoveUser(user);
        return;
    }
    
    sendType = MessageSendType::LOCAL;
    CommandResponse cres{};
    printf("cannot use /leaveRoom\n");
    cres.set_response("Cannot use this command now.");
    cres.set_type(CommandType::INVALID);
    envelope.set_payload(cres.SerializeAsString());

    printf("sending command response to client\n");
    server->Send(envelope, sendType, senderSocket);
}