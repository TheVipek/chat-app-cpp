# CMakeList.txt : CMake project for chat_app_server, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("chat_app_server")
find_package(Protobuf CONFIG REQUIRED)

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ../protobuf/communication.proto)

add_executable (chat_app_server "main.cpp" "server.cpp" "server.h" ${PROTO_SRCS} ${PROTO_HDRS})

target_include_directories(chat_app_server PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Protobuf_INCLUDE_DIRS}
)

target_link_libraries(chat_app_server
    PRIVATE protobuf::libprotobuf
)
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET chat_app_server PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
