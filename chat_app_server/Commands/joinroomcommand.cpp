#include "joinroomcommand.h"

void JoinRoomCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    file_logger->info("Join Room Command Process");
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);

    auto user = server->GetUser(senderSocket);

    MessageSendType sendType;

    if (user->id() == -1)
    {
        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        file_logger->warn("Invalid usage");
        cres.set_response("You need to /setnick first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }
    else if (user->connectedroomid() != -1)
    {
        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        file_logger->warn("Invalid usage");
        cres.set_response("You need to leave your current room first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }
    else if (creq.requestparameters().size() == 1)
    {
        std::string param1 = creq.requestparameters()[0];

        if (param1.empty())
        {
            sendType = MessageSendType::LOCAL;
            CommandResponse cres{};
            file_logger->warn("Invalid usage");
            cres.set_response("Invalid usage of /joinRoom (eg. /joinRoom #Lobby)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());
        }
        else
        {
            if (server->HasRoom(param1))
            {
                auto roomContainer = server->GetRoomContainer(param1);

                user->set_connectedroomid(roomContainer->room->id());

                sendType = MessageSendType::LOCAL;

                CommandResponse cres{};

                std::string newUserData;
                user->SerializeToString(&newUserData);

                cres.set_response(newUserData);
                cres.set_type(CommandType::JOIN_ROOM);
                envelope.set_payload(cres.SerializeAsString());

                std::string envelopeParsed;
                envelope.SerializeToString(&envelopeParsed);


                file_logger->info("Send to client");
                server->Send(envelope, sendType, senderSocket);
                roomContainer->AddUser(user);
                file_logger->info("Add user {} to room {}", user->name(), roomContainer->room->name());


                Envelope envelope2{};
                sendType = MessageSendType::WITHIN_ROOM_EXCEPT_THIS;
                envelope2.set_type(MessageType::USER_JOIN_ROOM);
                std::string msg = user->name() + " joined channel";
                envelope2.set_payload(msg);

                server->Send(envelope2, sendType, senderSocket);

                return;

            }

            sendType = MessageSendType::LOCAL;
            CommandResponse cres{};
            file_logger->warn("Invalid usage");
            cres.set_response("Room doesn't exist.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());
        }
    }

    server->Send(envelope, sendType, senderSocket);
}