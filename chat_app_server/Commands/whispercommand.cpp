#include "whispercommand.h"
#include "../Helpers.h"
void WhisperCommand::Execute(const CommandRequest& creq, const SOCKET senderSocket, Server* server)
{
    Envelope envelope{};
    envelope.set_type(MessageType::COMMAND);
    envelope.set_sendtype(MessageSendType::LOCAL);
    SPDLOG_LOGGER_INFO(file_logger, "Whisper");

    auto fromUser = server->GetUser(senderSocket);

    if (fromUser->id() == -1)
    {
        CommandResponse cres{};
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        cres.set_response("You need to /setnick first.");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());

        server->Send(envelope, senderSocket);

        return;
    }

    if (creq.requestparameters().size() == 2)
    {
        //name#ID
        std::string sendToUser = creq.requestparameters()[0];

        if (sendToUser.empty())
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /whisper (eg. /whisper userName#ID Hello) ID should be only numbers");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }

        int posOfDelimiter;
        if (sendToUser.empty() || (posOfDelimiter = sendToUser.find('#')) == std::string::npos)
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /whisper (eg. /whisper userName#ID Hello) ID should be only numbers");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }
        std::string userID = sendToUser.substr(posOfDelimiter + 1, sendToUser.size() - 1);

        if (!Helpers::isInteger(userID))
        {
            CommandResponse cres{};
            SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
            cres.set_response("Invalid usage of /whisper (eg. /whisper userName#ID Hello) ID should be only numbers");
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }

        int userID_int = std::stoi(userID);

        if (!server->UserExists(userID_int))
        {
            CommandResponse cres{};
            std::string response = std::format("User {} doesn't exist. ", sendToUser);
            SPDLOG_LOGGER_WARN(file_logger, response);
            cres.set_response(response);
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }

        auto toUser = server->GetUser(userID_int);

        std::string dataToSend = creq.requestparameters()[1];

        if (dataToSend.empty()) {
            CommandResponse cres{};
            std::string response = std::format("Message is empty", userID);
            SPDLOG_LOGGER_WARN(file_logger, response);
            cres.set_response(response);
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }
        
        SOCKET toSocket = server->GetUserSocket(toUser);

        if (toSocket == -1)
        {
            CommandResponse cres{};
            std::string response = std::format("User {} doesn't exist. ", sendToUser);
            SPDLOG_LOGGER_WARN(file_logger, response);
            cres.set_response(response);
            cres.set_type(CommandType::INVALID);
            envelope.set_payload(cres.SerializeAsString());

            SPDLOG_LOGGER_INFO(file_logger, "Send to client");
            server->Send(envelope, senderSocket);

            return;
        }

        auto fromUserFullName = fromUser->name() + "#" + std::to_string(fromUser->id());
        WhisperData whisperData{};



        whisperData.set_from(fromUserFullName);
        whisperData.set_message(dataToSend);

        CommandResponse cres{};
        cres.set_response(whisperData.SerializeAsString());
        cres.set_type(CommandType::USER_WHISPER);
        
        envelope.set_sendtype(MessageSendType::DIRECT);
        envelope.set_payload(cres.SerializeAsString());

        SPDLOG_LOGGER_INFO(file_logger, "Send to client");
        SOCKET sockets[] = { senderSocket, toSocket };
        server->SendDirect(envelope, sockets, 2);

        return;
    }
    else 
    {
        CommandResponse cres{};
        SPDLOG_LOGGER_WARN(file_logger, "Invalid usage");
        cres.set_response("Invalid usage of /whisper (eg. /whisper userName#ID Hello) ID should be only numbers");
        cres.set_type(CommandType::INVALID);
        envelope.set_payload(cres.SerializeAsString());
    }

    SPDLOG_LOGGER_INFO(file_logger, "Send to client");
    server->Send(envelope, senderSocket);
}