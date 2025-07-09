#include "createroom.h"
#include "../Helpers.h"
void CreateRoom::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    SPDLOG_LOGGER_INFO(file_logger, "Create Room Command Process");
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    auto user = server->GetUser(senderSocket);

    MessageSendType sendType;

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
        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        cres.set_response("You need to leave your current room first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());

        server->Send(envelope, senderSocket);

        return;
    }

    //CREATION WITHOUT PASSWORD
    if (creq.requestparameters().size() == 3)
    {
        std::string param1 = creq.requestparameters()[0];
        if (server->HasRoom(param1))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Room already exists, please use different name.");
            CommandResponse cres{};
            cres.set_response("Room already exists, please use different name.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        std::string param2 = creq.requestparameters()[1];

        if (!Helpers::isInteger(param2))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Second parameter should be number.");
            CommandResponse cres{};
            cres.set_response("Second parameter should be number.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        int param2_int = std::stoi(param2);
        if (param2_int < 1)
        {
            SPDLOG_LOGGER_WARN(file_logger, "Second parameter value should be higher than 1.");
            CommandResponse cres{};
            cres.set_response("Second parameter value should be higher than 1.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        std::string param3 = creq.requestparameters()[2];

        if (!Helpers::isInteger(param3))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Third parameter should be number. (0 = not public, 1 = public)");
            CommandResponse cres{};
            cres.set_response("Third parameter should be number. (0 = not public, 1 = public)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        int param3_int = std::stoi(param3);

        if (param3_int != 0 && param3_int != 1)
        {
            SPDLOG_LOGGER_WARN(file_logger, "Third parameter should be number. (0 = not public, 1 = public)");
            CommandResponse cres{};
            cres.set_response("Third parameter should be number. (0 = not public, 1 = public)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        auto roomContainer = server->CreateRoom(param1, param2_int, param3_int, "", true);

        //notification succes created
        CommandResponse cres{};
        SPDLOG_LOGGER_INFO(file_logger, "You succesfully created room.");
        cres.set_response("You succesfully created room.");
        cres.set_type(CommandType::CREATE_ROOM);
        envelope.set_payload(cres.SerializeAsString());
        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);


        // connect to newly created room
        server->UpdateExistingUserData(senderSocket, -1, "", roomContainer->room->id(), roomContainer->room->name());
        std::string newUserData;
        user->SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::JOIN_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        std::string envelopeParsed;
        envelope.SerializeToString(&envelopeParsed);


        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);
        SPDLOG_LOGGER_INFO(file_logger, "Created and connected user {} to room {}", user->name(), roomContainer->room->name());
        return;
    }
    //CREATION WITH PASSWORD
    else if (creq.requestparameters().size() == 4)
    {
        std::string param1 = creq.requestparameters()[0];
        if (server->HasRoom(param1))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Room already exists, please use different name.");
            CommandResponse cres{};
            cres.set_response("Room already exists, please use different name.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        std::string param2 = creq.requestparameters()[1];

        if (!Helpers::isInteger(param2))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Second parameter should be number.");
            CommandResponse cres{};
            cres.set_response("Second parameter should be number.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }
        int param2_int = std::stoi(param2);
        if (param2_int < 1)
        {
            SPDLOG_LOGGER_WARN(file_logger, "Second parameter value should be higher than 1.");
            CommandResponse cres{};
            cres.set_response("Second parameter value should be higher than 1.");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        std::string param3 = creq.requestparameters()[2];

        if (!Helpers::isInteger(param3))
        {
            SPDLOG_LOGGER_WARN(file_logger, "Third parameter should be number. (0 = not public, 1 = public)");
            CommandResponse cres{};
            cres.set_response("Third parameter should be number. (0 = not public, 1 = public)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }
        int param3_int = std::stoi(param3);

        if (param3_int != 0 && param3_int != 1)
        {
            SPDLOG_LOGGER_WARN(file_logger, "Third parameter should be number. (0 = not public, 1 = public)");
            CommandResponse cres{};
            cres.set_response("Third parameter should be number. (0 = not public, 1 = public)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            server->Send(envelope, senderSocket);

            return;
        }

        std::string param4 = creq.requestparameters()[3];

        auto roomContainer = server->CreateRoom(param1, param2_int, param3_int, param4, true);

        //notification succes created
        CommandResponse cres{};
        SPDLOG_LOGGER_INFO(file_logger, "You succesfully created room.");
        cres.set_response("You succesfully created room.");
        cres.set_type(CommandType::CREATE_ROOM);
        envelope.set_payload(cres.SerializeAsString());
        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);


        // connect to newly created room
        server->UpdateExistingUserData(senderSocket, -1, "", roomContainer->room->id(), roomContainer->room->name());
        std::string newUserData;
        user->SerializeToString(&newUserData);

        cres.set_response(newUserData);
        cres.set_type(CommandType::JOIN_ROOM);
        envelope.set_payload(cres.SerializeAsString());

        std::string envelopeParsed;
        envelope.SerializeToString(&envelopeParsed);


        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        server->Send(envelope, senderSocket);
        SPDLOG_LOGGER_INFO(file_logger, "Created and connected user {} to room {}", user->name(), roomContainer->room->name());
        return;

    }
   

    //INVALID
    CommandResponse cres{};
    SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
    cres.set_response("You need to pass 3 arguments for room creation without password (NAME MAX_CONNECTIONS IS_PUBLIC) or with password (NAME MAX_CONNECTIONS IS_PUBLIC PASSWORD)");
    cres.set_type(CommandType::INVALID);
    envelope.set_payload(cres.SerializeAsString());

    server->Send(envelope, senderSocket);
}