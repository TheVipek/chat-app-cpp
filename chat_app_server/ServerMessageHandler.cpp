#include "ServerMessageHandler.h"

#include "Commands/helpcommand.h"
#include "Commands/setnickcommand.h"
#include "Commands/joinroomcommand.h"
#include "Commands/roomlistcommand.h"
#include "Commands/leaveroomcommand.h"

ServerMessageHandler::ServerMessageHandler(Server* _server, std::shared_ptr<spdlog::logger> _file_logger) {
	server = _server;
    file_logger = _file_logger;
    commands["/help"] = std::make_unique<HelpCommand>(_file_logger);
    commands["/setnick"] = std::make_unique<SetNickCommand>(_file_logger);
    commands["/joinRoom"] = std::make_unique<JoinRoomCommand>(_file_logger);
    commands["/leaveRoom"] = std::make_unique<LeaveRoomCommand>(_file_logger);
    commands["/roomList"] = std::make_unique<RoomListCommand>(_file_logger);
}

void ServerMessageHandler::HandleMessage(const Envelope& envelope, const SOCKET senderSocket)
{
    file_logger->info("Handle Message for socket: {}", senderSocket);
    switch (envelope.type())
    {
        case MessageType::CHAT_MESSAGE:
        {

            std::string envelopeParsed;
            envelope.SerializeToString(&envelopeParsed);


            auto user = server->GetUser(senderSocket);
            if (user->id() != -1 && user->connectedroomid() != -1)
            {
                file_logger->info("Sending Chat Message from {} user", user->name());

                int totallySendTo = 0;
                
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
                                file_logger->warn("Problem with sending data to socket {} error {}", dest, WSAGetLastError());
                            }
                            else 
                            {
                                totallySendTo++;
                            }
                        }
                    }
                }

                file_logger->info("Sent to {} users", totallySendTo);
            }
            break;
        }
        case MessageType::PING: 
        {
            if (envelope.type() != MessageType::PING)
            {
                closesocket(senderSocket);
                server->connectedUsers.erase(senderSocket);
                server->lastPingTime.erase(senderSocket);
            }
            else
            {
                file_logger->info("Pong socket {}", senderSocket);
            }

            break;
        }
        case MessageType::COMMAND:
        {
            file_logger->info("Handling command");

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
                    file_logger->warn("Invalid command: {} from socket {}", req, senderSocket);


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

