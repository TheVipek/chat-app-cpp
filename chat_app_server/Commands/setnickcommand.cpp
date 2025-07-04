#include "setnickcommand.h"

void SetNickCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    MessageSendType sendType;
    file_logger->info("Set Nick Command Process");
    if (creq.requestparameters().size() == 1)
    {
        std::string proposedUserName = creq.requestparameters()[0];

        if (proposedUserName.empty())
        {
            sendType = MessageSendType::LOCAL;
            CommandResponse cres{};
            file_logger->warn("Invalid usage");
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

            file_logger->info("Set proposed nickname {}", proposedUserName.c_str());

            user->set_name(proposedUserName);

            sendType = MessageSendType::LOCAL;
            CommandResponse cres{};

           
            std::string newUserData;
            user->SerializeToString(&newUserData);

            cres.set_response(newUserData);
            cres.set_type(CommandType::NICKNAME);
            envelope.set_payload(cres.SerializeAsString());
        }
    }
    else {
        file_logger->warn("Invalid usage");

        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        cres.set_response("Invalid usage of /setnick (eg. /setnick SIGMA)");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }

    file_logger->info("Send to client");
    server->Send(envelope, sendType, senderSocket);
}