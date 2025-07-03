#include "ServerMessageHandler.h"

#include "Commands/helpcommand.h"
#include "Commands/setnickcommand.h"
#include "Commands/joinroomcommand.h"
#include "Commands/roomlistcommand.h"
#include "Commands/leaveroomcommand.h"

ServerMessageHandler::ServerMessageHandler(Server* _server) {
	server = _server;

    commands["/help"] = std::make_unique<HelpCommand>();
    commands["/setnick"] = std::make_unique<SetNickCommand>();
    commands["/joinRoom"] = std::make_unique<JoinRoomCommand>();
    commands["/leaveRoom"] = std::make_unique<LeaveRoomCommand>();
    commands["/roomList"] = std::make_unique<RoomListCommand>();
}

void ServerMessageHandler::HandleMessage(const Envelope& envelope, const SOCKET senderSocket)
{
    switch (envelope.type())
    {
        case MessageType::CHAT_MESSAGE:
        {
            std::string envelopeParsed;
            envelope.SerializeToString(&envelopeParsed);


            auto user = server->GetUser(senderSocket);
            if (user->id() != -1 && user->connectedroomid() != -1)
            {
                printf("handling chat message to users (%d)\n", server->writeSet.fd_count);
                for (int j = 0; j < server->writeSet.fd_count; j++)
                {
                    SOCKET dest = server->writeSet.fd_array[j];

                    //send to all except server and sender
                    if (dest != server->serverSocket && dest != senderSocket)
                    {
                        auto userSendTo = server->GetUser(dest);
                        if (user->connectedroomid() == userSendTo->connectedroomid())
                        {
                            if (send(dest, envelopeParsed.data(), envelopeParsed.size(), 0) == -1)
                            {
                                printf("send %d \n", WSAGetLastError());
                            }
                        }
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
                std::string req = creq.request();
                if (commands.contains(req))
                {
                    commands[req]->Execute(creq, senderSocket, server);
                }
                else 
                {
                    Envelope envelope{};
                    envelope.set_type(MessageType::COMMAND);
                    printf("invalid command\n");

                    CommandResponse cres{};
                    cres.set_response("Invalid Command.");
                    cres.set_type(CommandType::INVALID);
                    envelope.set_payload(cres.SerializeAsString());

                    server->Send(envelope, MessageSendType::LOCAL, senderSocket);
                }
            }
            break;
        }
        default:
            break;
    }
}

