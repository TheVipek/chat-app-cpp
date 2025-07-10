#include "ServerMessageHandler.h"

#include "Commands/helpcommand.h"
#include "Commands/setnickcommand.h"
#include "Commands/joinroomcommand.h"
#include "Commands/roomlistcommand.h"
#include "Commands/leaveroomcommand.h"
#include "Commands/createroom.h"
#include "Commands/whispercommand.h"

ServerMessageHandler::ServerMessageHandler(Server* _server, std::shared_ptr<spdlog::logger> _file_logger) {
	server = _server;
    file_logger = _file_logger;

    //love command pattern
    commands["/help"] = std::make_unique<HelpCommand>(_file_logger);
    commands["/setnick"] = std::make_unique<SetNickCommand>(_file_logger);
    commands["/joinRoom"] = std::make_unique<JoinRoomCommand>(_file_logger);
    commands["/leaveRoom"] = std::make_unique<LeaveRoomCommand>(_file_logger);
    commands["/roomList"] = std::make_unique<RoomListCommand>(_file_logger);
    commands["/createRoom"] = std::make_unique<CreateRoom>(_file_logger);
    commands["/whisper"] = std::make_unique<WhisperCommand>(_file_logger);

}

ServerMessageHandler::~ServerMessageHandler()
{
    commands.clear();
}

void ServerMessageHandler::HandleMessage(const Envelope& envelope, const SOCKET senderSocket)
{
    SPDLOG_LOGGER_INFO(file_logger, "Handle Message for socket: {}", senderSocket);
    if (!server->UserExists(senderSocket))
    {
        SPDLOG_LOGGER_WARN(file_logger, "User you're sending from doesn't exist (?)");
        Envelope errorEnvelope{};

        errorEnvelope.set_type(MessageType::CHAT_MESSAGE);
        errorEnvelope.set_sendtype(MessageSendType::LOCAL);
        errorEnvelope.set_payload("User you're sending from doesn't exist (?)");
        server->Send(envelope, senderSocket);
        return;
    }


    switch (envelope.type())
    {
        case MessageType::CHAT_MESSAGE:
        {

            auto user = server->GetUser(senderSocket);
            if (user->id() != -1 && user->connectedroomid() != -1)
            {
                SPDLOG_LOGGER_INFO(file_logger, "Sending Chat Message from {} user", user->name());
                server->Send(envelope, senderSocket);
            }
            break;
        }
        case MessageType::PING: 
        {
            SPDLOG_LOGGER_INFO(file_logger, "Pong socket {}", senderSocket);
            break;
        }
        case MessageType::COMMAND:
        {
            SPDLOG_LOGGER_INFO(file_logger, "Handling command");

            
            CommandRequest creq{};
            creq.ParseFromString(envelope.payload());
            std::string req = creq.request();
            if (commands.contains(req))
            {
                commands[req]->Execute(creq, senderSocket, server);
            }
            else
            {
                Envelope errorEnvelope{};
                errorEnvelope.set_type(MessageType::COMMAND);
                errorEnvelope.set_sendtype(MessageSendType::LOCAL);
                SPDLOG_LOGGER_WARN(file_logger, "Invalid command: {} from socket {}", req, senderSocket);


                CommandResponse cres{};
                cres.set_response("Invalid Command.");
                cres.set_type(CommandType::INVALID);
                errorEnvelope.set_payload(cres.SerializeAsString());

                server->Send(errorEnvelope, senderSocket);
            }
            break;
        }
        default:
            break;
    }
}

