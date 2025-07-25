## TCP Chat APP
Implemented in C++ using windows sockets for communication, ftxui for client-side UI and spdlog for logging.

## Features

* Realtime communication
* Simple TUI for client
* Extensible Command System: Supports a variety of commands implemented using the **Command Pattern** for easy extendability and robust testability. Available commands include:
  * ``/help``: Displays available commands.
  *  ``/createRoom [name] [maxConnections] [isPublic] [password (optional)]``: Creates a new chat room.
  * ``/joinRoom [name]``: Joins an existing chat room.
  * ``/leaveRoom``: Leaves the current chat room.
  * ``/roomList``: Shows all available chat rooms (only public).
  * ``/setNick``: Changes your name.
  * ``/whisper [userName]#[userID] [message]`` Sends a private message to a specific user.
* Server configuration: via a dedicated configuration file, including endpoint, predefined rooms, maximum connections, and ping timeout.
* Client configuration: via a dedicated configuration file, allowing to specify server endpoint.
* Logging: Utilizes `spdlog` for application logging.

## Dependencies
* Protobuf
* spdlog
* ftxui

## Preqrequisites
* C++ Compiler: Primarly developed and tested with MSVC
* CMake: 3.10 or higher
* VCPKG: dependencies

## Getting started

### 1. Clone repository
```bash
git clone https://github.com/TheVipek/chat-app-cpp.git
```
### 2. Configure VCPKG
```bash
vcpkg install protobuf spdlog ftxui
```
### 3. Build with CMake (for both Client and Server) 
```bash
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[PathToVcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```
### 4. Run server
```bash
.\chat_app_server.exe
```
### 5. Run client
```bash
.\chat_app_client.exe
```
### 6. Enjoy!

## Screenshots
### Server
<img width="2418" height="1213" alt="Zrzut ekranu 2025-07-13 150711" src="https://github.com/user-attachments/assets/7c14ed5c-deda-4724-af07-670f45b065ea" />

### Client
<img width="2554" height="1389" alt="Zrzut ekranu 2025-07-13 150513" src="https://github.com/user-attachments/assets/3ba181f4-6913-4116-9d6b-a933d93f2c52" />

