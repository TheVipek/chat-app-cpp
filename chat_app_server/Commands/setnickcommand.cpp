#include "setnickcommand.h"

void SetNickCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    SPDLOG_LOGGER_INFO(file_logger, "Set Nick Command Process");
    if (creq.requestparameters().size() == 1)
    {
        std::string proposedUserName = creq.requestparameters()[0];

        if (proposedUserName.empty())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /setnick (eg. /setnick SIGMA)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());
        }
        else
        {
            auto user = server->GetUser(senderSocket);

            if (user->id() == -1)
            {
                user->set_id(server->GetNewUserIdentifier());
            }

            SPDLOG_LOGGER_INFO(file_logger, "Set proposed nickname {}", proposedUserName.c_str());

            user->set_name(proposedUserName);
            CommandResponse cres{};

           
            std::string newUserData;
            user->SerializeToString(&newUserData);

            cres.set_response(newUserData);
            cres.set_type(CommandType::NICKNAME);
            envelope.set_payload(cres.SerializeAsString());
        }
    }
    else {
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        CommandResponse cres{};
        cres.set_response("Invalid usage of /setnick (eg. /setnick SIGMA)");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }

    SPDLOG_LOGGER_INFO(file_logger, "Send to client");
    server->Send(envelope, senderSocket);
}