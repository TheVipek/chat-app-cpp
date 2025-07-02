#include "ServerMessageHandler.h"

ServerMessageHandler::ServerMessageHandler(Server* _server) {
	server = _server;
}

void ServerMessageHandler::HandleMessage(const Envelope& envelope, const SOCKET& senderSocket)
{
    switch (envelope.type())
    {
        case MessageType::CHAT_MESSAGE:
        {
            printf("handling chat message to users (%d)\n", server->writeSet.fd_count);
            for (int j = 0; j < server->writeSet.fd_count; j++)
            {
                SOCKET dest = server->writeSet.fd_array[j];

                //send to all except server and sender
                if (dest != server->serverSocket && dest != senderSocket)
                {
                    if (send(dest, server->messageBuffer, server->receivedBytes, 0) == -1)
                    {
                        printf("send %d \n", WSAGetLastError());
                    }
                }
            }
            break;
        }
        case MessageType::COMMAND:
        {
            printf("handling command\n");

            if (FD_ISSET(senderSocket, &server->writeSet))
            {
                CommandRequest creq{};
                creq.ParseFromString(envelope.payload());


                //in very simple way for now, just want to have it working correctly
                Envelope envelope{};
                envelope.set_type(MessageType::COMMAND);

                std::string req = creq.request();
                if (req.starts_with("/help"))
                {
                    printf("help process\n");
                    envelope.set_sendtype(MessageSendType::Unicast);
                    CommandResponse cres{};
                    cres.set_response("/setnick \"userName\" - set your nickname \n, / joinRoom roomID - join room with specific name, / leaveRoom - leave your current room.");
                    cres.set_type(CommandType::HELP);
                    envelope.set_payload(cres.SerializeAsString());
                }
                else if (req.starts_with("/setnick"))
                {
                    size_t firstQuotePos = req.find('\"');
                    size_t lastQuotePos = req.rfind('\"');

                    if (firstQuotePos != std::string::npos &&
                        lastQuotePos != std::string::npos &&
                        firstQuotePos < lastQuotePos) 
                    {
                        std::string proposedUserName = req.substr(firstQuotePos + 1, lastQuotePos - firstQuotePos - 1);
                        if (proposedUserName.empty())
                        {
                            envelope.set_sendtype(MessageSendType::Unicast);
                            CommandResponse cres{};
                            printf("invalid usage of setnick\n");
                            cres.set_response("Invalid usage of /setnick command, you need to specify your nickname");
                            cres.set_type(CommandType::INVALID);
                            envelope.set_payload(cres.SerializeAsString());
                        }
                        else 
                        {
                            printf("setnick process\n");

                            ClientUser& user = server->connectedUsers[senderSocket];

                            if (user.id() == -1)
                            {
                                user.set_id(GetNewUserIdentifier());
                            }

                            printf("set proposed nickname: %s", proposedUserName.c_str());
                            user.set_name(proposedUserName);

                            envelope.set_sendtype(MessageSendType::Unicast);
                            CommandResponse cres{};

                            std::string newUserData;
                            user.SerializeToString(&newUserData);

                            cres.set_response(newUserData);
                            cres.set_type(CommandType::NICKNAME);
                            envelope.set_payload(cres.SerializeAsString());
                        }
                    }
                    else 
                    {
                        printf("invalid usage of setnick\n");

                        envelope.set_sendtype(MessageSendType::Unicast);
                        CommandResponse cres{};
                        cres.set_response("Invalid usage of /setnick command, you need to put your nickname within \"X\"  (eg. \"SIGMA\")");
                        cres.set_type(CommandType::INVALID);
                        envelope.set_payload(cres.SerializeAsString());
                    }
                }
                else if (req.starts_with("/joinRoom"))
                {

                }
                else if (req.starts_with("/leaveRoom"))
                {

                }
                else 
                {
                    printf("invalid command\n");

                    envelope.set_sendtype(MessageSendType::Unicast);
                    CommandResponse cres{};
                    cres.set_response("Invalid Command.");
                    cres.set_type(CommandType::INVALID);
                    envelope.set_payload(cres.SerializeAsString());
                }

                std::string envelopeParsed;
                envelope.SerializeToString(&envelopeParsed);

                printf("sending command response to client");
                if (send(senderSocket, envelopeParsed.data(), envelopeParsed.size(), 0) == SOCKET_ERROR)
                {
                    std::cerr << "send " << WSAGetLastError() << "\n";
                }
            }
            break;
        }
        default:
            break;
    }
}

int ServerMessageHandler::GetNewUserIdentifier() {
    int uniqueID = (rand() % 10000000);
    printf("uniqueID: %d", uniqueID);
    while (true) {
        for (auto u : server->connectedUsers)
        {
            if (u.second.id() == uniqueID)
            {
                uniqueID = (rand() % 1000000);
                printf("new uniqueID: %d", uniqueID);
                continue;
            }
        }
        break;
    }

    return uniqueID;
}