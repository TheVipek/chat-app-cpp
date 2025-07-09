#define MAX_CONNECTIONS 200
#include <iostream>
#include "server.h"
#include "ServerMessageHandler.h"


Server::Server(std::shared_ptr<spdlog::logger> _file_logger) {
    file_logger = _file_logger;
    srand(time(0));
    connectedUsers = std::map<SOCKET, ClientUser*>();
    userByID = std::unordered_map<int, ClientUser*>();
    roomContainers = std::map<std::string, RoomContainer*>();
    initialized = false;
    this->maxConnections = MAX_CONNECTIONS;
    serverMessageHandler = new ServerMessageHandler(this, _file_logger);
}

Server::~Server() {
    closesocket(serverSocket);

    delete(serverMessageHandler);
    delete(serverConfig);

    auto tempConnectedUsers = std::map<SOCKET, ClientUser*>(connectedUsers);

    for (auto u : tempConnectedUsers)
    {
        RemoveUser(u.first, false);
    }
    connectedUsers.clear();
    for (auto r : roomContainers)
    {
        delete(r.second);
    }
    roomContainers.clear();

    userByID.clear();
}

bool Server::Initialize(AdvancedServerConfig* _serverConfig) {
    if (initialized)
        return false;
    
    serverConfig = _serverConfig;

    for (const Room& room : serverConfig->rooms())
    {
        Room* roomData = new Room();
        roomData->CopyFrom(room);
        RoomContainer* container = new RoomContainer(roomData, false);

        roomContainers.insert({ roomData->name(), container });
    }

    FD_ZERO(&writeSet);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        SPDLOG_LOGGER_ERROR(file_logger, "Socket creation failed {}", WSAGetLastError());

        WSACleanup();
        return false;
    }

    serverAddr = {};
    serverAddr.sin_family = AF_INET;
    if (serverConfig->server().address().empty()) {
        serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
    }
    else {
        in_addr addr;

        if (inet_pton(AF_INET, serverConfig->server().address().c_str(), &addr) == -1)
        {
            SPDLOG_LOGGER_ERROR(file_logger, "Invalid IPv4 address, setting as local");

            serverAddr.sin_addr.s_addr = INADDR_ANY; // local address ip
        }
        else {
            serverAddr.sin_addr = addr;
        }

    }
    serverAddr.sin_port = htons(serverConfig->server().port());

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        SPDLOG_LOGGER_ERROR(file_logger, "Bind failed {}", WSAGetLastError());

        closesocket(serverSocket);
        return false;
    }

    SPDLOG_LOGGER_INFO(file_logger, "Server listening on port {}", serverConfig->server().port());

    if (listen(serverSocket, maxConnections) == SOCKET_ERROR) {
        SPDLOG_LOGGER_ERROR(file_logger, "Listen failed: {}", WSAGetLastError());
        closesocket(serverSocket);
        return false;
    }



    initialized = true;
    fdmax = serverSocket;

    return true;
}

bool Server::IsInitialized()
{
    return initialized;
}

void Server::PingClients() {
    Envelope ping_envelope{};
    ping_envelope.set_type(MessageType::PING);
    ping_envelope.set_sendtype(MessageSendType::LOCAL);
    ping_envelope.set_payload("");

    auto now = std::chrono::steady_clock::now();

    std::vector<SOCKET> socketsToRemove;

    for (int i = 0; i < writeSet.fd_count; i++) {
        SOCKET soc = writeSet.fd_array[i];
        if (UserExists(soc))
        {
            if (now - lastTimeActivity[soc] > std::chrono::seconds(pingTimeoutSeconds)) {
                auto user = connectedUsers[soc];
                SPDLOG_LOGGER_INFO(file_logger, "Ping socket {}, user {}#{}, payload {}", soc, user->name(), user->id(), ping_envelope.payload());
                bool failed = Send(ping_envelope, soc).size() > 0;
                if (failed)
                {
                    SPDLOG_LOGGER_WARN(file_logger, "SOCKET {} marked as to be kicked, due to network issues.", soc);
                    socketsToRemove.push_back(soc);
                    continue;
                }
                UpdateClientActivity(soc);
            }
        }
    }

    for (auto soc : socketsToRemove)
    {
        RemoveUser(soc, true);
    }
}

void Server::UpdateClientActivity(SOCKET sender) {
    auto now = std::chrono::steady_clock::now();
    lastTimeActivity[sender] = now;
}

void Server::Run() {
    if (!initialized)
        return;



    while (true)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);

        for(auto soc : connectedUsers)
        {
            FD_SET(soc.first, &readSet);
            FD_SET(soc.first, &writeSet);
        }

        //always include server, so incoming 'connect()' would be handled properly
        FD_SET(serverSocket, &readSet);
        FD_SET(serverSocket, &writeSet);


        //nfds is beign ignored on winsocks, that's why i pass 0 
        //i have no plans on testing it on linux
        if (select(0, &readSet, &writeSet, NULL, NULL) == SOCKET_ERROR)
        {
            SPDLOG_LOGGER_ERROR(file_logger, "SELECT ERROR {}", WSAGetLastError());
            throw WSAGetLastError();
        }

        for (u_int i = 0; i < readSet.fd_count; i++)
        {
            SOCKET senderSocket = readSet.fd_array[i];

            //handle connection to server
            if (senderSocket == serverSocket)
            {
                addrlen = sizeof(clientAddr);

                if ((newfd = accept(serverSocket, (sockaddr*)&clientAddr, &addrlen)) == INVALID_SOCKET)
                {
                    SPDLOG_LOGGER_ERROR(file_logger, "SOCKET ACCEPT ERROR {}", WSAGetLastError());
                }
                else
                {
                    AddStartUpUser(newfd);
                    fdmax = (newfd > fdmax) ? newfd : fdmax;

                    char ip[INET_ADDRSTRLEN];
                    SPDLOG_LOGGER_INFO(file_logger, "New connection from {} on socket {} ", inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)), newfd);
                }
            }
            //handle data from client
            else
            {
                if (UserExists(senderSocket))
                {
                    int receivedBytes;

                    char frameSizeBuffer[sizeof(uint32_t)];
                    receivedBytes = recv(senderSocket, frameSizeBuffer, sizeof(uint32_t), 0);

                    if (receivedBytes <= 0)
                    {
                        SPDLOG_LOGGER_ERROR(file_logger, "Lost connection to socket {}? error {} bytes received {}", senderSocket, WSAGetLastError(), receivedBytes);
                        RemoveUser(senderSocket, true);
                        continue;
                    }
                    uint32_t frameSize = *reinterpret_cast<uint32_t*>(frameSizeBuffer);
                    frameSize = ntohl(frameSize);


                    int allReceivedBytes = 0;

                    std::vector<char> buffer(frameSize);
                    receivedBytes = recv(senderSocket, buffer.data(), frameSize, 0);
                    if (receivedBytes <= 0)
                    {
                        SPDLOG_LOGGER_ERROR(file_logger, "Lost connection to socket {}? error {} bytes received {}", senderSocket, WSAGetLastError(), receivedBytes);
                        RemoveUser(senderSocket, true);
                        continue;
                    }
                   

                    UpdateClientActivity(senderSocket);
                    Envelope envelope{};
                    envelope.ParseFromString(buffer.data());
                    serverMessageHandler->HandleMessage(envelope, senderSocket);
                }
            }
        }

        PingClients();
    }
}

void Server::Stop() {
    if (!initialized)
        return;
}

std::vector<SOCKET> Server::Send(Envelope envelope, SOCKET senderSocket)
{
    std::vector<SOCKET> failedSockets;

    std::string envelopeParsed;
    envelope.SerializeToString(&envelopeParsed);

    // Make a copy of the write set to avoid race conditions
    fd_set currentWriteSet;
    FD_ZERO(&currentWriteSet);
    for (int i = 0; i < writeSet.fd_count; i++) {
        FD_SET(writeSet.fd_array[i], &currentWriteSet);
    }

    auto sendToSocket = [&](SOCKET dest) {
        if (!FD_ISSET(dest, &currentWriteSet)) return false;

        std::string envelopeParsed = envelope.SerializeAsString();
        uint32_t envelopeSize = static_cast<uint32_t>(envelopeParsed.size());
        uint32_t networkOrderSize = htonl(envelopeSize);

        // Send length
        const char* frameData = reinterpret_cast<const char*>(&networkOrderSize);
        int frameTotalSent = 0;
        while (frameTotalSent < 4) {
            int n = send(dest, frameData + frameTotalSent, 4 - frameTotalSent, 0);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) return false;
                SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", dest, err);
                return true;
            }
            frameTotalSent += n;
        }

        // Send payload
        const char* envelopeBuffer = envelopeParsed.data();
        int totalSent = 0;
        while (totalSent < envelopeSize) {
            int n = send(dest, envelopeBuffer + totalSent, envelopeSize - totalSent, 0);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) return false;
                SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", dest, err);
                return true;
            }
            totalSent += n;
        }
        return false;
        };

    auto msgSendType = envelope.sendtype();
    auto* userSender = GetUser(senderSocket);

    for (int j = 0; j < currentWriteSet.fd_count; j++) {
        SOCKET dest = currentWriteSet.fd_array[j];
        if (dest == serverSocket) continue;

        bool shouldSend = false;

        switch (msgSendType) {
        case MessageSendType::LOCAL:
            shouldSend = (dest == senderSocket);
            break;
        case MessageSendType::WITHIN_ROOM:
            shouldSend = (userSender->connectedroomid() == GetUser(dest)->connectedroomid());
            break;
        case MessageSendType::WITHIN_ROOM_EXCEPT_THIS:
            shouldSend = (dest != senderSocket) &&
                (userSender->connectedroomid() == GetUser(dest)->connectedroomid());
            break;
        case MessageSendType::GLOBAL:
            shouldSend = true;
            break;
        }

        if (shouldSend && sendToSocket(dest)) {
            failedSockets.push_back(dest);
        }
    }

    return failedSockets;
}

std::vector<SOCKET> Server::SendDirect(Envelope envelope, SOCKET targetSockets[], int count)
{
    std::vector<SOCKET> failedSockets;
    std::string envelopeParsed;
    envelope.SerializeToString(&envelopeParsed);

    // Make a copy of the write set to avoid race conditions
    fd_set currentWriteSet;
    FD_ZERO(&currentWriteSet);
    for (int i = 0; i < writeSet.fd_count; i++) {
        FD_SET(writeSet.fd_array[i], &currentWriteSet);
    }

    auto sendToSocket = [&](SOCKET dest) {
        if (!FD_ISSET(dest, &currentWriteSet)) return false;

        std::string envelopeParsed = envelope.SerializeAsString();
        uint32_t envelopeSize = static_cast<uint32_t>(envelopeParsed.size());
        uint32_t networkOrderSize = htonl(envelopeSize);

        // Send length
        const char* frameData = reinterpret_cast<const char*>(&networkOrderSize);
        int frameTotalSent = 0;
        while (frameTotalSent < 4) {
            int n = send(dest, frameData + frameTotalSent, 4 - frameTotalSent, 0);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) return false;
                SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", dest, err);
                return true;
            }
            frameTotalSent += n;
        }

        // Send payload
        const char* envelopeBuffer = envelopeParsed.data();
        int totalSent = 0;
        while (totalSent < envelopeSize) {
            int n = send(dest, envelopeBuffer + totalSent, envelopeSize - totalSent, 0);
            if (n == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAEWOULDBLOCK) return false;
                SPDLOG_LOGGER_ERROR(file_logger, "Send error to socket {}: {}", dest, err);
                return true;
            }
            totalSent += n;
        }
        return false;
        };

    bool sent = false;

    for(int i = 0; i < count; i++)
    {
        auto s = targetSockets[i];
        if (s != serverSocket)
        {
            sent = sendToSocket(s);

            if (!sent)
            {
                failedSockets.push_back(s);
            }
        }
    }
    


    return failedSockets;
}

int Server::GetNewUserIdentifier() {
    int uniqueID = (rand() % 10000000);
    while (true) {
        for (auto u : connectedUsers)
        {
            if (u.second->id() == uniqueID)
            {
                uniqueID = (rand() % 1000000);
                continue;
            }
        }
        break;
    }

    return uniqueID;
}

bool Server::UserExists(SOCKET socket)
{
    return connectedUsers.contains(socket);
}
bool Server::UserExists(int userID)
{
    return userByID.contains(userID);
}
ClientUser* Server::GetUser(SOCKET socket) 
{
    if (UserExists(socket))
    {
        return connectedUsers[socket];
    }
    return nullptr;
}
ClientUser* Server::GetUser(int userID)
{
    if (UserExists(userID))
    {
        return userByID[userID];
    }
    return nullptr;
}
std::vector<ClientUser> Server::GetAllUsersTemp()
{
    std::vector<ClientUser> users;
    for (auto u : connectedUsers)
    {
        auto instance = u.second;
        if (instance->id() == -1)
            continue;

        ClientUser user{};

        user.set_id(instance->id());
        user.set_name(instance->name());
        user.set_connectedroomid(instance->connectedroomid());
        user.set_roomname(instance->roomname());
        users.push_back(user);
    }

    return users;
}
SOCKET Server::GetUserSocket(ClientUser* client)
{
    for (auto v : connectedUsers)
    {
        if (v.second->id() == client->id())
        {
            return v.first;
        }
    }

    return -1;
}
void Server::AddStartUpUser(SOCKET sock)
{

    ClientUser* newUser = new ClientUser();
    newUser->set_id(-1);
    newUser->set_name("UnknownUser");
    newUser->set_connectedroomid(-1);
    newUser->set_roomname("");
    connectedUsers.insert({ sock, newUser });
    //skip adding to userByID
}
void Server::UpdateExistingUserData(SOCKET sock, int id, std::string name, int roomID, std::string roomName)
{
    if (UserExists(sock))
    {
        auto user = connectedUsers[sock];

        if (id != -1 && user->id() != id)
        {
            if (userByID.contains(user->id()))
            {
                userByID.erase(user->id());
            }
            user->set_id(id);
            userByID.insert({ user->id(), user});
        }

        if (!name.empty() && user->name() != name)
        {
            user->set_name(name);
        }

        if (user->connectedroomid() != roomID)
        {
            if (user->connectedroomid() != -1)
            {
                auto oldRoomContainer = GetRoomContainer(user->connectedroomid());
                if (oldRoomContainer != nullptr)
                {
                    oldRoomContainer->RemoveUser(user);
                }
            }

            auto newRoomContainer = GetRoomContainer(roomID);

            if (newRoomContainer != nullptr)
            {
                user->set_connectedroomid(roomID);
                user->set_roomname(newRoomContainer->room->name());
                newRoomContainer->AddUser(user);
            }
        }

        SendUpdateOfAllUsers();
    }
}
void Server::RemoveUser(SOCKET socket, bool notify)
{
    if (UserExists(socket))
    {
        auto user = connectedUsers[socket];
        if (user->connectedroomid() != -1)
        {
            if (notify) {
                //i will create more advanced handling for server if needed in future
                Envelope envelope2{};
                envelope2.set_type(MessageType::USER_LEAVE_ROOM);
                envelope2.set_sendtype(MessageSendType::WITHIN_ROOM_EXCEPT_THIS);
                std::string msg = user->name() + "has been kicked from the server due to network error.";
                envelope2.set_payload(msg);
                Send(envelope2, socket);
            }

            auto roomContainer = GetRoomContainer(user->connectedroomid());
            roomContainer->RemoveUser(user);
        }
        connectedUsers.erase(socket);

        if (userByID.contains(user->id()))
        {
            userByID.erase(user->id());
        }

        if (lastTimeActivity.contains(socket))
        {
            lastTimeActivity.erase(socket);
        }

        delete user;
    }

    closesocket(socket);

    if (notify)
    {
        SendUpdateOfAllUsers();
    }
}

void Server::SendUpdateOfAllUsers()
{
    Envelope envelope2{};
    envelope2.set_type(MessageType::ALL_USERS_UPDATE);
    envelope2.set_sendtype(MessageSendType::GLOBAL);

    auto users = GetAllUsersTemp();
    ClientUserList cul {};
    for (const auto& user : users) {
        *cul.add_users() = user; // Add each user to the list
    }

    std::string serialized;
    cul.SerializeToString(&serialized);

    envelope2.set_payload(serialized);
    Send(envelope2, serverSocket);
}
bool Server::HasRoom(std::string roomName) {
    return roomContainers.contains(roomName);
}
int Server::MaxRoomID()
{
    int maxID = -1;

    for (auto& k : roomContainers)
    {
        int roomID = k.second->room->id();
        maxID = maxID < roomID ? roomID : maxID;
    }

    return maxID;
}

RoomContainer* Server::GetRoomContainer(std::string roomName) {
    return roomContainers[roomName];
}
RoomContainer* Server::GetRoomContainer(int roomID) {
    for (auto& k : roomContainers)
    {
        auto roomContainer = k.second;
        if (roomContainer->room->id() == roomID)
        {
            return roomContainer;
        }
    }

    return nullptr;
}
std::vector<RoomContainer*> Server::GetRoomContainers()
{
    std::vector<RoomContainer*> _roomContainers = std::vector<RoomContainer*>();

    for (auto& k : roomContainers)
    {
        _roomContainers.push_back(k.second);
    }

    return _roomContainers;
}
RoomContainer* Server::CreateRoom(std::string name, int maxConnections, bool isPublic, std::string password, bool destroyOnEmpty)
{
    Room* newRoom = new Room();
    newRoom->set_name(name);
    newRoom->set_maxconnections(maxConnections);
    newRoom->set_ispublic(isPublic);

    if (password.empty())
    {
        newRoom->set_haspassword(false);
    }
    else 
    {
        newRoom->set_haspassword(true);
        newRoom->set_password(password);
    }
    int roomID = MaxRoomID() + 1;

    newRoom->set_id(roomID);
    RoomContainer* container = new RoomContainer(newRoom, destroyOnEmpty);

    roomContainers[newRoom->name()] = container;

    return container;
}
void Server::RemoveRoom(std::string name)
{
    if (roomContainers.contains(name))
    {
        auto room = roomContainers[name];
        roomContainers.erase(name);

        delete room;
    }
}