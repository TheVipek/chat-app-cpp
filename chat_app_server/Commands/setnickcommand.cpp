#include "setnickcommand.h"

void SetNickCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    MessageSendType sendType;

    if (creq.requestparameters().size() == 1)
    {
        std::string proposedUserName = creq.requestparameters()[0];

        if (proposedUserName.empty())
        {
            sendType = MessageSendType::LOCAL;
            CommandResponse cres{};
            printf("invalid usage of setnick\n");
            cres.set_response("Invalid usage of /setnick (eg. /setnick SIGMA)");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());
        }
        else
        {
            printf("setnick process\n");

            auto user = server->GetUser(senderSocket);

            if (user->id() == -1)
            {
                user->set_id(server->GetNewUserIdentifier());
            }

            printf("set proposed nickname: %s", proposedUserName.c_str());
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
        printf("invalid usage of setnick\n");

        sendType = MessageSendType::LOCAL;
        CommandResponse cres{};
        cres.set_response("Invalid usage of /setnick (eg. /setnick SIGMA)");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }

    printf("sending command response to client\n");
    server->Send(envelope, sendType, senderSocket);
}