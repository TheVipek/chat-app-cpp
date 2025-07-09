#include "joinroomcommand.h"

void JoinRoomCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    SPDLOG_LOGGER_INFO(file_logger, "Join Room Command Process");
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    auto user = server->GetUser(senderSocket);


    if (user->id() == -1)
    {
        CommandResponse cres{};
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        cres.set_response("You need to /setnick first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());

        server->Send(envelope, senderSocket);

        return;
    }
    
    if (user->connectedroomid() != -1)
    {
        CommandResponse cres{};
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        cres.set_response("You need to leave your current room first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());

        server->Send(envelope, senderSocket);

        return;
    }
    
    
    //try to join without password
    if (creq.requestparameters().size() == 1)
    {
        std::string param1 = creq.requestparameters()[0];

        if (param1.empty())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /joinRoom (eg. /joinRoom #Lobby)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (!server->HasRoom(param1))
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Room doesn't exist.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        auto roomContainer = server->GetRoomContainer(param1);

        if (roomContainer->room->haspassword())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "This room requires password.");
            cres.set_response("This room requires password. (eg. /joinRoom #Lobby password123)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (roomContainer->usersInRoom.size() >= roomContainer->room->maxconnections())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Cannot join, room has reached max capacity.");
            cres.set_response("Cannot join, room has reached max capacity.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }


        SPDLOG_LOGGER_INFO(file_logger, "Add user {} to room {}", user->name(), roomContainer->room->name());
        server->UpdateExistingUserData(senderSocket, -1, "", roomContainer->room->id(), roomContainer->room->name());

        CommandResponse cres{};

        std::string newUserData;
        user->SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::JOIN_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);




        envelope.Clear();
        envelope.set_type(MessageType::USER_JOIN_ROOM);
        envelope.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
        std::string msg = user->name() + " joined channel";
        envelope.set_payload(msg);

        server->Send(envelope, senderSocket);

        return;
        
    }

    //try to join with password
    if (creq.requestparameters().size() == 2)
    {
        std::string param1 = creq.requestparameters()[0];
        std::string param2 = creq.requestparameters()[1];

        if (param1.empty())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /joinRoom (eg. /joinRoom #Lobby)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (!server->HasRoom(param1))
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Room doesn't exist.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        auto roomContainer = server->GetRoomContainer(param1);

        if (!roomContainer->room->haspassword())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "This room doesn't require password.");
            cres.set_response("This room doesn't require password.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (param2.empty())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /joinRoom (eg. /joinRoom #Lobby password123)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (roomContainer->room->password() != param2)
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid password");
            cres.set_response("Invalid password");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        if (roomContainer->usersInRoom.size() >= roomContainer->room->maxconnections())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Cannot join, room has reached max capacity.");
            cres.set_response("Cannot join, room has reached max capacity.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        
        SPDLOG_LOGGER_INFO(file_logger, "Add user {} to room {}", user->name(), roomContainer->room->name());
        server->UpdateExistingUserData(senderSocket, -1, "", roomContainer->room->id(), roomContainer->room->name());


        CommandResponse cres{};

        std::string newUserData;
        user->SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::JOIN_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);

        envelope.Clear();

        envelope.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
        envelope.set_type(MessageType::USER_JOIN_ROOM);
        std::string msg = user->name() + " joined channel";
        envelope.set_payload(msg);

        server->Send(envelope, senderSocket);

        return;

    }

}